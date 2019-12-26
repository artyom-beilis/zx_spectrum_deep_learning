    5 REM "Params very fast config"
   10 LET kernels=4
   15 LET ksize=3
   20 LET itersize=4
   30 LET clsno=2
   35 LET batch=64: LET epochs=2
   40 LET blr=0.01 : LET blr2=0.01
   50 LET wd=0.0005
   60 LET beta=0.90
   61 REM "Params End"
   62 LET intsz=8-ksize+1
   64 LET poolsz=INT(intsz/2)
   66 LET fltsz=poolsz*poolsz*kernels
   67 LET wdcomp=1-wd
   68 DIM m(3,clsno,fltsz) : REM "IP Matrix"
   70 DIM b(3,clsno) : REM "IP Offset"
   75 DIM k(3,kernels,ksize,ksize) : REM "Kernels"
   80 DIM o(3,kernels) : REM "Kernel Offset"
   90 DIM c(2,kernels,intsz,intsz) : REM "conv_res"
  100 DIM p(2,fltsz) : REM "pool res"
  110 DIM r(2,clsno) : REM "probs"
  120 DIM s(fltsz) : REM "pool mask"
  130 DIM d(8,8) : REM "digit"
  200 REM "Functions"
  210 DEF FN g(s)=s*(RND+RND+RND+RND+RND+RND+RND+RND+RND+RND+RND+RND-6)
  220 DEF FN r(x)=(x>0)*x
  300 REM "loading data"
  305 LOAD ""SCREEN$
  310 LET datarow=0
  311 REM "Initalization"
  312 PRINT AT 21,0;"INI ";
  315 LET sigma=1/(kernels*ksize*ksize)
  320 FOR n=1 TO kernels: FOR r=1 TO ksize: FOR c=1 TO ksize: LET k(1,n,r,c)=FN g(sigma): NEXT c: NEXT r: NEXT n
  325 LET sigma=2/(fltsz + clsno)
  330 FOR r=1 TO clsno: FOR c=1 TO fltsz: LET m(1,r,c)=FN g(sigma): NEXT c: NEXT r
  400 REM "train loop"
  410 FOR e=1 TO epochs
  420 IF e>=2 THEN LET blr=blr2
  422 GO SUB 9000: LET start=time
  425 LET acc=0: LET count=0
  427 LET iter=0
  430 FOR b=0 TO batch-1
  435 LET loss=0
  440 FOR d=0 TO clsno-1
  450 GO SUB 8000: REM "Get pixel from d,b"
  455 LET mark=24: GO SUB 8050: REM "Mark"
  460 GO SUB 3500 : REM "Forward"
  470 GO SUB 4000 : REM "Backward"
  480 LET acc=acc + accres
  490 LET count=count+1
  500 LET loss=loss+lossres
  510 LET mark=16*(1+accres): GO SUB 8050: REM "Mark Res"
  520 NEXT d
  525 LET iter=iter+1
  530 IF iter=itersize THEN GO SUB 5000: LET iter=0: REM "apply update"
  540 NEXT b
  542 GO SUB 9000: LET pass=time-start
  545 LET acc=INT(acc / count * 1000)/10
  550 PRINT AT 20,0;"Epoch=";e;" Acc=";acc;"% time ";INT(pass+0.5);"m "
  560 NEXT e
  999 REM "Test"
 1000 LET datarow=4
 1422 GO SUB 9000: LET start=time
 1425 LET acc=0: LET count=0
 1430 FOR b=0 TO batch-1
 1440 FOR d=0 TO clsno-1
 1450 GO SUB 8000: REM "Get pixel from d,b"
 1455 LET mark=24: GO SUB 8050: REM "Mark"
 1460 GO SUB 3500 : REM "Forward"
 1480 LET acc=acc + accres
 1490 LET count=count+1
 1510 LET mark=16*(1+accres): GO SUB 8050 : REM "Mark Res"
 1520 NEXT d
 1540 NEXT b
 1542 GO SUB 9000: LET pass=time-start
 1545 LET acc=INT(acc / count * 1000)/10
 1550 PRINT AT 20,0;"Test Acc=";acc;"% time ";INT(pass+0.5);"m ";
 1590 GO TO 9999: REM "EEEENNNNDDD"
 3498 REM "Forward Prop"
 3499 REM "Conv FWD"
 3500 PRINT AT 21,0;"FW  ";
 3505 FOR r=1 TO intsz: FOR c=1 TO intsz : FOR n=1 TO kernels
 3510 LET sum=o(1,n)
 3520 FOR i=1 TO ksize: FOR j=1 TO ksize
 3530 LET sum=sum + k(1,n,i,j) * d(r+i-1,c+j-1)
 3540 NEXT j: NEXT i
 3550 LET c(1,n,r,c) = sum
 3560 NEXT n: NEXT c: NEXT r
 3599 REM "Max Pool Relu"
 3600 LET pos=1
 3605 FOR n=1 TO kernels: FOR r=1 TO poolsz*2 STEP 2 : FOR c=1 TO poolsz*2 STEP 2
 3610 LET index=0 : LET maxv=c(1,n,r,c): 
 3620 IF c(1,n,r  ,c+1) > maxv THEN LET maxv=c(1,n,r  ,c+1) : LET index=1
 3630 IF c(1,n,r+1,c  ) > maxv THEN LET maxv=c(1,n,r+1,c  ) : LET index=2
 3640 IF c(1,n,r+1,c+1) > maxv THEN LET maxv=c(1,n,r+1,c+1) : LET index=3
 3650 LET s(pos)=index : LET p(1,pos)=FN r(maxv) : LET pos=pos+1
 3660 NEXT c: NEXT r: NEXT n
 3699 REM "IP Forward"
 3700 FOR i=1 TO clsno
 3710 LET sum=b(1,i)
 3720 FOR j=1 TO fltsz
 3730 LET sum=sum+p(1,j)*m(1,i,j)
 3740 NEXT j
 3750 LET r(1,i)=sum
 3760 NEXT i
 3799 REM "Loss"
 3800 LET maxind=0 : LET maxv=r(1,1): LET sum=0
 3810 FOR i=2 TO clsno: 
 3820 IF r(1,i) > maxv THEN LET maxv=r(1,i) : LET maxind=i-1
 3830 NEXT i
 3840 FOR i=1 TO clsno
 3850 LET tgt = (i-1) = d
 3860 LET sdiff=tgt-r(1,i): LET sum=sum+sdiff*sdiff
 3870 NEXT i
 3880 LET lossres=0.5*sum: LET accres=(maxind=d)
 3890 RETURN
 3900 REM "END OF LOSS FW"
 3998 REM "BACK PROP"
 3999 REM "Loss Backward"
 4000 PRINT AT 21,0;"BW  ";
 4005 FOR i=1 TO clsno
 4010 LET tgt = (i-1) = d
 4020 LET r(2,i) = r(1,i) - tgt
 4030 NEXT i
 4049 REM "IP Backward"
 4050 FOR k=1 TO clsno
 4060 LET b(2,k) = b(2,k) + r(2,k)
 4070 NEXT k
 4080 FOR j=1 TO fltsz : LET p(2,j) = 0: NEXT j
 4090 FOR i=1 TO clsno : FOR j=1 TO  fltsz
 4100 LET m(2,i,j) = m(2,i,j) + p(1,j)*r(2,i)
 4110 LET p(2,j)=p(2,j) + m(1,i,j) * r(2,i)
 4120 NEXT j: NEXT i
 4200 REM "Pool/ReLU backward"
 4210 FOR k=1 TO fltsz
 4220 IF p(1,k) <= 0 THEN LET p(2,k) = 0
 4230 NEXT k
 4240 LET pos=1
 4250 FOR k=1 TO kernels:
 4260 FOR r=1 TO intsz STEP 2 : FOR c=1 TO intsz STEP 2
 4270 LET indx=s(pos)
 4280 LET topv=p(2,pos)
 4290 LET c(2,k,r  ,c  ) = (indx=0) * topv
 4300 LET c(2,k,r  ,c+1) = (indx=1) * topv
 4310 LET c(2,k,r+1,c  ) = (indx=2) * topv
 4320 LET c(2,k,r+1,c+1) = (indx=3) * topv
 4330 LET pos=pos+1
 4340 NEXT c: NEXT r
 4350 NEXT k
 4360 REM "Conv Bias BW"
 4400 FOR n=1 TO kernels
 4410 LET sum=0
 4420 FOR i=1 TO intsz: FOR j=1 TO intsz
 4430 LET sum=sum+c(2,n,i,j)
 4440 NEXT j : NEXT i
 4450 LET o(2,n) = o(2,n) + sum
 4460 NEXT n
 4499 REM "Conv BW"
 4500 FOR r=1 TO intsz: FOR c=1 TO intsz: FOR n=1 TO kernels
 4510 FOR i=1 TO ksize : FOR j=1 TO ksize
 4520 LET k(2,n,i,j) = k(2,n,i,j) + d(r+i-1,c+j-1) * c(2,n,r,c)
 4530 NEXT j: NEXT i
 4540 NEXT n: NEXT c: NEXT r
 4550 RETURN
 4599 REM "End of BW"
 4998 REM "Apply Update"
 4999 REM "IP matrix update"
 5001 PRINT AT 21,0;"UPD   ";
 5002 FOR i=1 TO clsno
 5005 FOR j=1 TO fltsz
 5010 LET m(3,i,j) = m(3,i,j) * beta + m(2,i,j) * blr
 5020 LET m(1,i,j) = wdcomp * m(1,i,j) - m(3,i,j)
 5030 LET m(2,i,j) = 0
 5040 NEXT j
 5049 REM "Matrix Offset"
 5050 LET b(3,i) = b(3,i) * beta + b(2,i) * blr
 5060 LET b(1,i) = wdcomp * b(1,i) - b(3,i)
 5070 LET b(2,i) = 0
 5080 NEXT i
 5099 REM "Kernel update"
 5100 FOR i=1 TO kernels
 5105 FOR j=1 TO ksize: FOR k=1 TO ksize 
 5110 LET k(3,i,j,k) = k(3,i,j,k) * beta + k(2,i,j,k) * blr
 5120 LET k(1,i,j,k) = wdcomp * k(1,i,j,k) - k(3,i,j,k)
 5130 LET k(2,i,j,k) = 0
 5140 NEXT k: NEXT j
 5149 REM "Kernel Offset"
 5150 LET o(3,i) = o(3,i) * beta + o(2,i) * blr
 5160 LET o(1,i) = wdcomp * o(1,i) - o(3,i)
 5170 LET o(2,i) = 0
 5180 NEXT i
 5190 RETURN
 5299 REM "End of Apply Update"
 7999 REM "Fill l with digit, b=0-63, d 0 to 9
 8000 LET row=((b>=32) + d*2 + datarow) * 8
 8010 LET col=((b>=32)*(b-32)+(b<32)*b) * 8
 8020 FOR r=0 TO 7: FOR c=0 TO 7
 8030 LET d(r+1,c+1)=POINT((col+c),(175-row-r))
 8035 REM PRINT AT 4+r,c;d(r+1,c+1);
 8040 NEXT c: NEXT r: RETURN
 8049 REM "Mark digit/batch"
 8050 LET row=((b>=32) + d*2 + datarow)
 8060 LET col=((b>=32)*(b-32)+(b<32)*b)
 8070 LET addr=16384+6144+row*32+col
 8080 POKE addr,mark
 8090 RETURN
 8999 REM "Get timer in minutes"
 9000 LET time=(PEEK 23672+256*(PEEK 23673+256*PEEK 23674))/3000
 9010 RETURN 
