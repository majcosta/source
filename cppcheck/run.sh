#!/usr/bin/env sh

cppcheck -j24 --cppcheck-build-dir=build --xml --enable=unusedFunction --inline-suppr --error-exitcode=1 --project=../build/compile_commands.json --suppressions-list=suppressions.txt 2> report/report.xml && cppcheck-htmlreport --file report/report.xml --report-dir=report --source-dir=..
