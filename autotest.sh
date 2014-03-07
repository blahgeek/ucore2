#!/bin/bash

BASE_COMMIT=a26c0705167d60f1f5f39784d8e3cd9866d3a205
if [ -f .base_commit ]; then
    BASE_COMMIT=`cat .base_commit`
fi
LABS=`git diff $BASE_COMMIT --stat | grep -o "lab[0-9]" | uniq`

if [ "$LABS" = "" ]; then
    echo "No solutions provided. Skip this time."
    exit 0
fi

failed=0

pwd=`pwd`
summary=$pwd/.score_summary

echo -n > $summary
for lab in $LABS; do
    pushd $lab > /dev/null
    if ! make grade > .score 2>&1; then
        failed=`echo $lab | grep -o [0-9]`
    fi
    score=`egrep -o "Score: [0-9]+/[0-9]+" .score`
    echo "$lab $score" >> $summary
    make clean > /dev/null
    popd > /dev/null
done

echo "Labs with changes detected: " $LABS
echo
echo "============================== Summary =============================="
cat $summary
rm $summary
echo

for lab in $LABS; do
    echo "================================ $lab ==============================="
    cat $lab/.score
    rm $lab/.score
done

echo $BASE_COMMIT > .base_commit
exit $failed
