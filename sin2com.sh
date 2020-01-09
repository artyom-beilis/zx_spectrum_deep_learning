FL=train_no_buffer.bas
cat $FL  | grep -v SPECTRUM | sed 's/REM CMD://'  | tr '[A-Z]' '[a-z]' | sed 's/go to/goto/g' | sed 's/go sub/gosub/g' | sed 's/let //g' >test.bas
#sed 's/(\([^)$,]*\),\([^),]*\),\([^),]*\),\([^),]*\))/(\1-1,\2-1,\3-1,\4-1)/g' | \
#sed 's/(\([^)$,]*\),\([^),]*\),\([^),]*\))/(\1-1,\2-1,\3-1)/g' | \
#sed 's/(\([^)$,]*\),\([^),]*\))/(\1-1,\2-1)/g' \
~/my/com/vice/bin/petcat -w2 -o test.prg -- test.bas
