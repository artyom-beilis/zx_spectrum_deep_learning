   10 CLEAR 25599
   15 LOAD ""CODE 
   17 REM "Load Train Data"
   20 LOAD ""SCREEN$
   25 GO SUB 1000: REM "Get epochs"
   27 LET trtime=0
   30 FOR e=0 TO (epochs-1)
   40 LET ep=e: GO SUB 200
   50 NEXT e
   55 REM "Test"
   60 LOAD ""SCREEN$
   70 LET ep=255
   80 GO SUB 200
   90 LET pass=trtime/epochs : GO SUB 600
  100 PRINT AT 21,0;"Avg Train Time: ";n$;" ";
  110 GO TO 9999
  200 PRINT AT 20,0;
  203 IF ep=255 THEN PRINT "Testing";
  206 IF ep<>255 THEN PRINT "Epoch=";ep+1;
  208 POKE 25599,ep
  209 GO SUB 300: LET start=time
  210 LET ac=(100/4096)*USR 25600
  220 GO SUB 300: LET pass=time-start
  225 LET trtime=trtime+pass
  230 LET val=ac: GO SUB 500: PRINT " Acc=";n$;" ";
  235 GO SUB 600: PRINT "time:";n$;" ";
  240 RETURN 
  300 LET time=(PEEK 23672+256*(PEEK 23673+256*PEEK 23674))/50
  310 RETURN 
  500 LET n$=STR$ val
  510 FOR i=LEN n$ TO 1 STEP -1
  520 IF n$(i)="." THEN LET n$=n$( TO i+1): RETURN 
  530 NEXT i
  540 RETURN 
  600 LET pass=INT(pass)
  605 LET min=INT(pass/60)
  610 LET sec=INT(pass - min*60)
  630 LET s$=STR$ sec
  640 IF LEN s$ <= 1 THEN LET s$="0"+s$
  650 LET n$=STR$ min+":"+s$
  660 RETURN
