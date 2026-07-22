#!/usr/bin/env python3
"""Measure what the #include graph costs to rebuild.

Every static library here is compiled once per application, so a header that
600 translation units can see is a header that, when touched, recompiles 600
objects four times over. This script measures that, and ranks the individual
#include lines whose removal would shrink it the most.

The metric is the number of (translation unit, header) pairs: how many times
some .cpp transitively pulls in some header. It is proportional to both the
preprocessing work of a full build and the blast radius of editing a header.

Include resolution mimics the build: the include path is flat (see the
include_directories() block in CMakeLists.txt), so an include is resolved by
file name alone, ignoring any directory part. Includes that do not resolve to
a file in the tree (system and toolchain headers) are ignored: we cannot
delete those anyway. Vendored code under ext/ is skipped for the same reason.

Usage:
    tools/include_graph.py                  summary + hottest headers
    tools/include_graph.py --edges [N]      rank the N most expensive includes
    tools/include_graph.py --edge A.h B.h   what deleting one include would cost
"""

import argparse
import os
import re
import sys
from collections import defaultdict

SKIP_DIRS = {".git", ".github", ".cache", "ext", "cppcheck", "tools"}
SKIP_DIR_PREFIXES = ("build",)  # build, build-debug, and any other build tree
SOURCE_SUFFIXES = (".cpp", ".c")
HEADER_SUFFIXES = (".h",)

INCLUDE_RE = re.compile(r'^\s*#\s*include\s*[<"]([^">]+)[">]', re.M)

# Identifiers a header declares. Deliberately loose: this only feeds a
# "which files would need a direct include" estimate, which the compiler then
# has the final word on. Erring towards too many names is the safe direction —
# it makes an include look used, and the worst case is that we leave a dead
# include in place.
DECL_RE = re.compile(
    r"""^\s*(?:
        \#\s*define\s+(\w+)
      | typedef\s+[^;{]*?\b(\w+)\s*;
      | (?:struct|class|enum|union)\s+(\w+)
      | extern\s+[\w\s\*&:<>,]*?\b(\w+)\s*[\(;\[]
      | [\w\s\*&:<>,]+?\b(\w+)\s*\([^;{]*\)\s*[;{]
    )""",
    re.M | re.X,
)

# Enumerators, which the declaration pattern above cannot see: much of the
# tunable game data is a single unnamed enum of several hundred constants.
ENUM_RE = re.compile(r"\benum\b[^{;]*\{([^}]*)\}", re.S)
ENUMERATOR_RE = re.compile(r"^\s*(\w+)")

# The name a braced typedef ends with, which the single-line patterns cannot
# see: `typedef struct { ... } DEALER_POSSIBLE_INV;` spans many lines. This
# also picks up names that merely follow a closing brace, which is harmless —
# an extra name can only make an include look used.
TYPEDEF_TAIL_RE = re.compile(r"\}\s*(\w+)\s*;")

COMMENT_RE = re.compile(r"//[^\n]*|/\*.*?\*/", re.S)

# Too common to be evidence that a file uses a particular header.
NOT_A_SYMBOL = {
    "void", "class", "struct", "union", "enum", "int", "char", "long", "short",
    "float", "double", "unsigned", "signed", "const", "static", "extern",
    "return", "sizeof", "true", "false", "NULL", "TRUE", "FALSE",
}


class Graph:
    def __init__(self, root):
        self.text = {}          # file name -> contents
        self.includes = {}      # file name -> [included file name, ...]
        self.sources = []       # file names of translation units
        self._scan(root)

    def _scan(self, root):
        for path, dirs, names in os.walk(root):
            dirs[:] = [d for d in dirs
                       if d not in SKIP_DIRS and not d.startswith(SKIP_DIR_PREFIXES)]
            for name in names:
                lower = name.lower()
                if not lower.endswith(SOURCE_SUFFIXES + HEADER_SUFFIXES):
                    continue
                with open(os.path.join(path, name), errors="ignore") as f:
                    self.text[lower] = f.read()
                if lower.endswith(SOURCE_SUFFIXES):
                    self.sources.append(lower)
        for name, text in self.text.items():
            self.includes[name] = [
                os.path.basename(i).lower() for i in INCLUDE_RE.findall(text)
            ]
        # Resolve: keep only edges that land on a file we actually have.
        self.edges = defaultdict(set)
        for name, included in self.includes.items():
            self.edges[name] = {i for i in included if i in self.text}
        self.sources.sort()

    def headers(self):
        return sorted(n for n in self.text if n.endswith(HEADER_SUFFIXES))

    def reachable(self, start, without=None):
        """Headers transitively included by `start`, ignoring one edge."""
        seen = set()
        stack = [start]
        while stack:
            node = stack.pop()
            for target in self.edges[node]:
                if without is not None and (node, target) == without:
                    continue
                if target not in seen:
                    seen.add(target)
                    stack.append(target)
        return seen

    def cost(self, without=None):
        """Total (translation unit, header) pairs."""
        return sum(len(self.reachable(s, without)) for s in self.sources)

    def fanout(self):
        """Header -> how many translation units can see it."""
        counts = defaultdict(int)
        for s in self.sources:
            for h in self.reachable(s):
                counts[h] += 1
        return counts

    def symbols(self, header):
        text = COMMENT_RE.sub(" ", self.text.get(header, ""))
        found = set()
        for match in DECL_RE.finditer(text):
            for group in match.groups():
                if group and len(group) > 3 and group not in NOT_A_SYMBOL:
                    found.add(group)
        for name in TYPEDEF_TAIL_RE.findall(text):
            if name not in NOT_A_SYMBOL:
                found.add(name)
        for body in ENUM_RE.findall(text):
            for entry in body.split(","):
                match = ENUMERATOR_RE.match(entry)
                if match and match.group(1) not in NOT_A_SYMBOL:
                    found.add(match.group(1))
        return found

    def mentions(self, name, symbols):
        """Whether a file refers to any of these names outside its includes.

        Comments are stripped: a header named in a comment is not a use of it.
        """
        body = COMMENT_RE.sub(" ", self.text[name])
        body = re.sub(r"^\s*#\s*include.*$", "", body, flags=re.M)
        return any(re.search(r"\b%s\b" % re.escape(s), body) for s in symbols)


def edge_report(graph, includer, included):
    """Cost and fallout of deleting one #include line."""
    edge = (includer, included)
    if included not in graph.edges[includer]:
        sys.exit("%s does not include %s" % (includer, included))

    before = {s: graph.reachable(s) for s in graph.sources}
    saved = 0
    orphaned = []
    for s in graph.sources:
        after = graph.reachable(s, without=edge)
        saved += len(before[s]) - len(after)
        if included in before[s] and included not in after:
            orphaned.append(s)

    symbols = graph.symbols(included)
    needs_include = [s for s in orphaned if graph.mentions(s, symbols)]
    return saved, orphaned, needs_include


def rank_edges(graph, limit, min_fanout):
    fanout = graph.fanout()
    hot = {h for h, c in fanout.items() if c >= min_fanout}
    ranked = []
    baseline = graph.cost()
    for includer in sorted(graph.edges):
        for included in sorted(graph.edges[includer]):
            if included not in hot:
                continue
            ranked.append((baseline - graph.cost(without=(includer, included)),
                           includer, included))
    ranked.sort(reverse=True)
    return ranked[:limit]


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--root", default=os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    parser.add_argument("--edges", nargs="?", type=int, const=25, metavar="N",
                        help="rank the N most expensive includes (slow)")
    parser.add_argument("--edge", nargs=2, metavar=("INCLUDER", "INCLUDED"),
                        help="report the cost of deleting one include")
    parser.add_argument("--min-fanout", type=int, default=100,
                        help="with --edges, only consider headers this many "
                             "translation units can see (default: 100)")
    parser.add_argument("--top", type=int, default=25,
                        help="how many hot headers to list (default: 25)")
    args = parser.parse_args()

    graph = Graph(args.root)

    if args.edge:
        includer, included = (a.lower() for a in args.edge)
        saved, orphaned, needs = edge_report(graph, includer, included)
        print("deleting  %s -> %s" % (includer, included))
        print("  pairs saved:                    %d" % saved)
        print("  translation units losing it:    %d" % len(orphaned))
        print("  ...of which mention its symbols: %d" % len(needs))
        for name in sorted(needs):
            print("      %s" % name)
        return

    fanout = graph.fanout()
    print("translation units: %d   headers: %d" % (len(graph.sources), len(graph.headers())))
    print("total (translation unit, header) pairs: %d" % graph.cost())
    print("\nheaders by how many translation units can see them:")
    for header, count in sorted(fanout.items(), key=lambda kv: -kv[1])[:args.top]:
        print("  %5d  %s" % (count, header))

    if args.edges:
        print("\nmost expensive includes:")
        for saved, includer, included in rank_edges(graph, args.edges, args.min_fanout):
            print("  -%-6d %s -> %s" % (saved, includer, included))


if __name__ == "__main__":
    main()
