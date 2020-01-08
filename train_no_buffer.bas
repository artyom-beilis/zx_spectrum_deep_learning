    5 REM "Params very fast config"
    7 LET sc=50*60 : REM SPECTRUM
    8 REM CMD: sc=60*60
    9 LET seed=1
   10 LET kr=4
   15 LET ks=3
   20 LET ts=4
   30 LET cn=2
   35 LET bt=64: LET ep=2
   36 REM LET kr=12: LET ts=1: LET ep=5: LET cn=10
   40 LET lr=0.01 
   50 LET wd=0.0005
   60 LET mo=0.90
   61 REM "Params End"
   62 LET iz=8-ks+1
   64 LET ps=INT(iz/2)
   66 LET fz=ps*ps*kr
   67 LET wc=1-wd
   68 DIM m(3,cn,fz) : REM "IP Matrix"
   70 DIM b(3,cn) : REM "IP Offset"
   75 DIM k(3,kr,ks,ks) : REM "Kernels"
   80 DIM o(3,kr) : REM "Kernel Offset"
   90 DIM c(2,kr,iz,iz) : REM "conv_res"
  100 DIM p(2,fz) : REM "pool res"
  110 DIM r(2,cn) : REM "probs"
  120 DIM s(fz) : REM "pool mask"
  130 DIM d(8,8) : REM "digit"
  200 REM "Functions"
  210 REM CMD: DEF FN g(s)=s*(RND(1)-0.5) 
  211 DEF FN g(s)=s*(RND-0.5) : REM SPECTRUM
  220 DEF FN r(x)=ABS(x>0)*x
  311 REM "Initalization"
  312 PRINT "INIT"
  315 LET sg=1/(kr*ks*ks)
  320 FOR n=1 TO kr: FOR r=1 TO ks: FOR c=1 TO ks
  321 LET seed=75*(seed+1): LET seed=seed-INT(seed/257)*257
  323 LET k(1,n,r,c)=sg*(seed/256-0.5)
  324 NEXT c: NEXT r: NEXT n
  325 LET sg=2/(fz + cn)
  330 FOR r=1 TO cn: FOR c=1 TO fz
  331 LET seed=75*(seed+1): LET seed=seed-INT(seed/257)*257
  335 LET m(1,r,c)=sg*(seed/256-0.5)
  350 NEXT c: NEXT r
  400 REM "train loop"
  410 FOR e=1 TO ep
  420 RESTORE
  422 GO SUB 9000: LET sr=time
  425 LET acc=0: LET co=0
  427 LET iter=0
  430 LET b=0
  440 LET d=0
  450 GO SUB 8000: REM "Get pixel from d,b"
  460 GO SUB 3500 : REM "Forward"
  470 GO SUB 4000 : REM "Backward"
  480 LET acc=acc + ares
  490 LET co=co+1
  510 GO SUB 8050: REM "Mark Res"
  520 LET d=d+1 : IF d < cn THEN GO TO 450
  525 LET iter=iter+1
  530 IF iter=ts THEN GO SUB 5000: LET iter=0: REM "apply update"
  540 LET b=b+1: IF b < bt THEN GO TO 440
  542 GO SUB 9000: LET pass=(time-sr)/sc
  545 LET acc=INT(acc / co * 1000)/10
  550 PRINT "Epoch=";e;" Acc=";acc;"% time ";INT(pass+0.5);"m "
  560 NEXT e
  999 REM "Test"
 1422 GO SUB 9000: LET sr=time
 1425 LET acc=0: LET co=0
 1430 FOR b=0 TO bt-1
 1440 FOR d=0 TO cn-1
 1450 GO SUB 8000: REM "Get pixel from d,b"
 1460 GO SUB 3500 : REM "Forward"
 1480 LET acc=acc + ares
 1490 LET co=co+1
 1510 GO SUB 8050 : REM "Mark Res"
 1520 NEXT d
 1540 NEXT b
 1542 GO SUB 9000: LET pass=(time-sr)/50/60
 1545 LET acc=INT(acc / co * 1000)/10
 1550 PRINT "Test Acc=";acc;"% time ";INT(pass+0.5);"m ";
 1590 GO TO 9999: REM "EEEENNNNDDD"
 3498 REM "Forward Prop"
 3500 REM "Conv FWD"
 3505 FOR r=1 TO iz: FOR c=1 TO iz 
 3510 FOR n=1 TO kr: LET c(1,n,r,c)=o(1,n): NEXT n
 3520 FOR i=1 TO ks: FOR j=1 TO ks
 3530 IF NOT d(r+i-1,c+j-1) THEN GO TO 3540 
 3532 FOR n=1 TO kr
 3533 LET c(1,n,r,c)=c(1,n,r,c)+k(1,n,i,j)
 3534 NEXT n
 3540 NEXT j: NEXT i
 3560 NEXT c: NEXT r
 3599 REM "Max Pool Relu"
 3600 LET pz=1
 3605 FOR n=1 TO kr: FOR r=1 TO ps*2 STEP 2 : FOR c=1 TO ps*2 STEP 2
 3610 LET in=0 : LET mv=c(1,n,r,c): 
 3620 IF c(1,n,r  ,c+1)>mv THEN LET mv=c(1,n,r  ,c+1) : LET in=1
 3630 IF c(1,n,r+1,c  )>mv THEN LET mv=c(1,n,r+1,c  ) : LET in=2
 3640 IF c(1,n,r+1,c+1)>mv THEN LET mv=c(1,n,r+1,c+1) : LET in=3
 3650 LET s(pz)=in : LET p(1,pz)=FN r(mv) : LET pz=pz+1
 3660 NEXT c: NEXT r: NEXT n
 3699 REM "IP Forward"
 3700 FOR i=1 TO cn
 3710 LET sum=b(1,i)
 3720 FOR j=1 TO fz
 3730 LET sum=sum+p(1,j)*m(1,i,j)
 3740 NEXT j
 3750 LET r(1,i)=sum
 3760 NEXT i
 3799 REM "Loss"
 3800 LET mind=0 : LET mv=r(1,1): LET sum=0
 3810 FOR i=2 TO cn: 
 3820 IF r(1,i) > mv THEN LET mv=r(1,i) : LET mind=i-1
 3830 NEXT i
 3840 FOR i=1 TO cn
 3850 LET tgt = ABS((i-1) = d)
 3860 LET sd=tgt-r(1,i): LET sum=sum+sd*sd
 3870 NEXT i
 3880 LET ares=ABS(mind=d)
 3890 RETURN
 3900 REM "END OF LOSS FW"
 3998 REM "BACK PROP"
 4000 REM "Loss Backward"
 4005 FOR i=1 TO cn
 4010 LET tgt = ABS((i-1) = d)
 4020 LET r(2,i) = r(1,i) - tgt
 4030 NEXT i
 4049 REM "IP Backward"
 4050 FOR k=1 TO cn
 4060 LET b(2,k) = b(2,k) + r(2,k)
 4070 NEXT k
 4080 FOR j=1 TO fz : LET p(2,j) = 0: NEXT j
 4090 FOR i=1 TO cn : FOR j=1 TO  fz
 4100 LET m(2,i,j) = m(2,i,j) + p(1,j)*r(2,i)
 4110 LET p(2,j)=p(2,j) + m(1,i,j) * r(2,i)
 4120 NEXT j: NEXT i
 4200 REM "Pool/ReLU backward"
 4210 FOR k=1 TO fz
 4220 IF p(1,k) <= 0 THEN LET p(2,k) = 0
 4230 NEXT k
 4240 LET pz=1
 4250 FOR k=1 TO kr:
 4260 FOR r=1 TO iz STEP 2 : FOR c=1 TO iz STEP 2
 4270 LET indx=s(pz)
 4280 LET tpv=p(2,pz)
 4290 LET c(2,k,r  ,c  ) = ABS(indx=0) * tpv
 4300 LET c(2,k,r  ,c+1) = ABS(indx=1) * tpv
 4310 LET c(2,k,r+1,c  ) = ABS(indx=2) * tpv
 4320 LET c(2,k,r+1,c+1) = ABS(indx=3) * tpv
 4330 LET pz=pz+1
 4340 NEXT c: NEXT r
 4350 NEXT k
 4360 REM "Conv Bias BW"
 4400 FOR n=1 TO kr
 4410 LET sum=0
 4420 FOR i=1 TO iz: FOR j=1 TO iz
 4430 LET sum=sum+c(2,n,i,j)
 4440 NEXT j : NEXT i
 4450 LET o(2,n) = o(2,n) + sum
 4460 NEXT n
 4499 REM "Conv BW"
 4500 FOR r=1 TO iz: FOR c=1 TO iz:
 4510 FOR i=1 TO ks : FOR j=1 TO ks
 4520 IF NOT d(r+i-1,c+j-1) THEN GO TO 4530 
 4525 FOR n=1 TO kr: LET k(2,n,i,j) = k(2,n,i,j) + c(2,n,r,c) : NEXT n
 4530 NEXT j: NEXT i
 4540 NEXT c: NEXT r
 4550 RETURN
 4599 REM "End of BW"
 4998 REM "Apply Update"
 5000 REM "IP matrix update"
 5002 FOR i=1 TO cn
 5005 FOR j=1 TO fz
 5010 LET m(3,i,j) = m(3,i,j) * mo + m(2,i,j) * lr
 5020 LET m(1,i,j) = wc * m(1,i,j) - m(3,i,j)
 5030 LET m(2,i,j) = 0
 5040 NEXT j
 5049 REM "Matrix Offset"
 5050 LET b(3,i) = b(3,i) * mo + b(2,i) * lr
 5060 LET b(1,i) = wc * b(1,i) - b(3,i)
 5070 LET b(2,i) = 0
 5080 NEXT i
 5099 REM "Kernel update"
 5100 FOR i=1 TO kr
 5105 FOR j=1 TO ks: FOR k=1 TO ks 
 5110 LET k(3,i,j,k) = k(3,i,j,k) * mo + k(2,i,j,k) * lr
 5120 LET k(1,i,j,k) = wc * k(1,i,j,k) - k(3,i,j,k)
 5130 LET k(2,i,j,k) = 0
 5140 NEXT k: NEXT j
 5149 REM "Kernel Offset"
 5150 LET o(3,i) = o(3,i) * mo + o(2,i) * lr
 5160 LET o(1,i) = wc * o(1,i) - o(3,i)
 5170 LET o(2,i) = 0
 5180 NEXT i
 5190 RETURN
 5299 REM "End of Apply Update"
 7999 REM "Fill l with digit, b=0-63, d 0 to 9"
 8000 READ v$ 
 8010 FOR r=0 TO 7
 8015 LET vl=16*(CODE v$(r*2+1) - CODE "a") + (CODE v$(r*2+2) - CODE "a") : REM SPECTRUM
 8016 REM POKE 16384+r*256,vl : REM SPECTRUM
 8017 REM CMD: vl=16*(ASC(mid$(v$,r*2+1,1))-ASC("a"))+(ASC(mid$(v$,r*2+2,1))-ASC("a"))
 8020 FOR c=0 TO 7
 8030 LET d(r+1,c+1)=(vl/2 <> INT(vl/2))
 8033 REM CMD: REM POKE 1024+r*40+c,35+d(r+1,c+1)*3
 8035 LET vl=INT(vl/2)
 8040 NEXT c: NEXT r
 8042 REM CMD: LET ff=FRE(0)
 8045 RETURN
 8049 REM "Mark digit/bt"
 8050 IF ares THEN PRINT "+";: RETURN
 8060 PRINT "-";
 9000 REM "Get timer in minutes"
 9005 LET time=(PEEK 23672+256*(PEEK 23673+256*PEEK 23674)) : REM SPECTRUM
 9010 RETURN 
9800 DATA "aaambmcccccedaaa","aaaeaeaibibadaaa"
9801 DATA "aaaebodkgcgediaa","aabibiaiaiaiaiaa"
9802 DATA "aaambmbededmdiaa","aaaaaaaiaiaiaiaa"
9803 DATA "aaambmdgcecmdiaa","aabaaaaiaiaaaiaa"
9804 DATA "aaamdocggggehmaa","aaaeaeaibibacaaa"
9805 DATA "aabidodcgcccdmaa","aaaiaidiaibmcaaa"
9806 DATA "aaambmdoccfehiaa","aaaiaiaiaiaibaaa"
9807 DATA "aabicceaeacaboaa","aaaeamaibibadaaa"
9808 DATA "aabmdmdececedmaa","aabiamaibibebaaa"
9809 DATA "aaambmbececediaa","aabidiaibabmbiaa"
9810 DATA "aaambmdececediaa","aaaaaiaiaiaabaaa"
9811 DATA "aabideccccccdmaa","aaaeamaibabadaaa"
9812 DATA "aaambmdececediaa","aaamamaiaibidaaa"
9813 DATA "aadaececeaccbmaa","aaaeamaiaibabaaa"
9814 DATA "aaaidohcgcegdiaa","aaaaaiaibabaaiaa"
9815 DATA "aaambadccccediaa","aabababaaiaiaiaa"
9816 DATA "aaaobodcgggmhiaa","aaaeambibibadaaa"
9817 DATA "aabmdcgcgcggdmaa","aaaiaiaiaiaiaaaa"
9818 DATA "aaaadkcceceediaa","aaamaibibidacaaa"
9819 DATA "aaambmdedecediaa","aaaiaiaiaaaabaaa"
9820 DATA "aaamaecgcgcebiaa","aaaiaibibibibiaa"
9821 DATA "aabmdoecececdmaa","aacadabiaiamaeaa"
9822 DATA "aadicggcecggdmaa","aaaiaiaiaibabaaa"
9823 DATA "aaaibecacacaceaa","aaaiaiaiaibabaaa"
9824 DATA "aabibedecececeaa","aaamamaiaibidaaa"
9825 DATA "aabidmccgcecdoaa","aabaaiaiaiaiaiaa"
9826 DATA "aabmbeaeaebebiaa","aababibibiaiaiaa"
9827 DATA "aaambecececediaa","aaaeamaibibadaaa"
9828 DATA "aacmbceaeaacccaa","aaaeamaibidacaaa"
9829 DATA "aabicggceaecdmaa","aaaiaiaibibibaaa"
9830 DATA "aaaaameaecemaaaa","aaaiaiaibibabaaa"
9831 DATA "aabmdggcgchgdmaa","aaambecmaiaidiaa"
9832 DATA "aabodgcccccgdmaa","aaaiaiaibibabaaa"
9833 DATA "aabicocccgcebiaa","aababibibiaiamaa"
9834 DATA "aabmbgdgccgmdiaa","aaaiamaibibibaaa"
9835 DATA "aabmbecececmdmaa","aaaiaiaibibadaaa"
9836 DATA "aaambmdccacediaa","aaaiaiaaaababaaa"
9837 DATA "aaaibodcccggdiaa","aaaiaiaiaababaaa"
9838 DATA "aaaobodccghmdiaa","aaaaaiaibibabaaa"
9839 DATA "aaambodggghmdiaa","aaaiaiaibibadaaa"
9840 DATA "aabmdggcgcggdmaa","aaaiaiaiaiaaaiaa"
9841 DATA "aaamdgccgcggdiaa","aaaeamaibibadaaa"
9842 DATA "aabmhehcgcdcboaa","aaaeaeaiaibadaaa"
9843 DATA "aabibmcfcecmdjaa","aaaeamambibadaaa"
9844 DATA "aaambkccgcemdaaa","aaaeamaibidacaaa"
9845 DATA "aadmdgccccggdmaa","aaaeamaibibidaaa"
9846 DATA "aaamdohcgcggdmaa","aabababibiaiaiaa"
9847 DATA "aadigmececgcbmaa","aaaaaiaiaiaibaaa"
9848 DATA "aaambkccggemhaaa","aabibibibibibiaa"
9849 DATA "aaaibccceghiaaaa","aabababababiamaa"
9850 DATA "aaambodcggfmhaaa","aaaeamaibibacaaa"
9851 DATA "aabmceccecegdmaa","aaaiaiaiaiaiaiaa"
9852 DATA "aabidegcgcgcdmaa","aababababiaiamaa"
9853 DATA "aaaaakbgcehicaaa","aaaiaiaibabaaaaa"
9854 DATA "aabmdebedecmdiaa","aaaaaiaibibibaaa"
9855 DATA "aaacboccggemhaaa","aaaeamaibibacaaa"
9856 DATA "aabibmdececebmaa","aaaeamaibibacaaa"
9857 DATA "aabadmccccdcboaa","aaaiaiaibibibiaa"
9858 DATA "aaaeaobccegihaaa","aaaiaiaiaiaiaaaa"
9859 DATA "aabadmcgcccebmaa","aaaeamaibibadaaa"
9860 DATA "aaaoamdecgeihaaa","aaaeaiaiaibabaaa"
9861 DATA "aaambodccegmdiaa","aaaibabababaaiaa"
9862 DATA "aaambmdgdgdmdiaa","aaaeaeaibabacaaa"
9863 DATA "aaaibmdcccceciaa","aaaiaiaiaiaiaiaa"
9900 DATA "aabibmdgcgdmdiaa","aaaaaiaiaababaaa"
9901 DATA "aabmcccccccedmaa","aaaeaiaiaibabaaa"
9902 DATA "aabmdecacccebeaa","aaaiaiaiaiaibiaa"
9903 DATA "aaamdohcgghmdiaa","aabababaaiaiaiaa"
9904 DATA "aabmbmbececedmaa","aababaaiaiaiaiaa"
9905 DATA "aabibebedecebiaa","aababaaiaiaiaiaa"
9906 DATA "aabadoeaecggbaaa","aaaiaibibibibiaa"
9907 DATA "aabmdgcccccgdmaa","aaaaaaaiaaaaaaaa"
9908 DATA "aaamboccccccdmaa","aabababiaiamamaa"
9909 DATA "aabicmeeececdmaa","aaaiaiaiaababaaa"
9910 DATA "aabibecececebiaa","aaaibibiaiaibaaa"
9911 DATA "aaaibebacacebiaa","aaaeamaibibabaaa"
9912 DATA "aaaibecececebmaa","aababibibibiamaa"
9913 DATA "aabmccgceceediaa","aaaababiaiaiaiaa"
9914 DATA "aabibodccccediaa","aaaeaiaibababaaa"
9915 DATA "aaagbkbcceeihaaa","aaaiaiaibibibiaa"
9916 DATA "aabidecececediaa","aaamaibibibadaaa"
9917 DATA "aabmdecececmdiaa","aaaiaiaiaibabaaa"
9918 DATA "aadiceececccboaa","aababaaaaiaiaiaa"
9919 DATA "aaaacmececemaaaa","aaaaaiaiaiaiaiaa"
9920 DATA "aabidecececedmaa","aababiaiaabiaiaa"
9921 DATA "aabibeaacacedmaa","aaaaaiaaaaaabaaa"
9922 DATA "aaamdgcceceohmaa","aaaiaiaiaiaiaiaa"
9923 DATA "aacacoeaecegdiaa","aaaiaiaiaiaiaiaa"
9924 DATA "aadmccebebccdmaa","aababaaaaiaiaaaa"
9925 DATA "aadmdocgcgdodmaa","aaaeambmbibibaaa"
9926 DATA "aabmdecececmdiaa","aabababaaiaiaiaa"
9927 DATA "aabaaicecaceceaa","aaaibibibibibiaa"
9928 DATA "aabmbccccecmdiaa","aaaeamaibababaaa"
9929 DATA "aaaibmbebecediaa","aaaiaiaiaiaiaiaa"
9930 DATA "aaambabaceceaiaa","aaaaaiaiaababaaa"
9931 DATA "aadacmaaacccbmaa","aaaaaaaiaababaaa"
9932 DATA "aabkcceaeaccdmaa","aaaiaiaiaababaaa"
9933 DATA "aabmdmcegcegdmaa","aaaiaiaiaiaiaiaa"
9934 DATA "aabmcacccccgbmaa","aaaababaaaaaaiaa"
9935 DATA "aadidogggggodmaa","aabiaiaiaibiaiaa"
9936 DATA "aabibecgcecediaa","aaaiaiaiaiaiaiaa"
9937 DATA "aaaibebebeaabiaa","aaaiaiaiaiaiaiaa"
9938 DATA "aabmcecaccccbmaa","aaaiaiaiaibabaaa"
9939 DATA "aabmbecececediaa","aaaiaiaiaiaiaiaa"
9940 DATA "aaaabkccecdmaaaa","aaamaibibababaaa"
9941 DATA "aabmbecececibiaa","aaaaaiaiaiaiaiaa"
9942 DATA "aabmcccccccediaa","aaaiaiaiaiaiaiaa"
9943 DATA "aaambcccceaeciaa","aaaiaiaibiaiaiaa"
9944 DATA "aaamceccgcccbmaa","aaaiaiaiaiaiaiaa"
9945 DATA "aaamdidmcedmbiaa","aaaiaiaiaiaiaiaa"
9946 DATA "aabadoececeebiaa","aaaiaiaiaiaiaiaa"
9947 DATA "aabibecececebmaa","aaaiaiaibiaiaiaa"
9948 DATA "aabiaececacebiaa","aaaiaiaibabibaaa"
9949 DATA "aadacmcccccgdiaa","aababibiaiaiaiaa"
9950 DATA "aaambeccccacdmaa","aaaeaeaibabacaaa"
9951 DATA "aaambgcccagedmaa","aaaiaiaiaiaiaiaa"
9952 DATA "aaaabiccecdmaaaa","aaaiaiaiaiaabaaa"
9953 DATA "aabmcaececeedmaa","aaaiaiaiaiaiaaaa"
9954 DATA "aadiccgccccgbmaa","aaaiaiaiaaaaaaaa"
9955 DATA "aabibecccccediaa","aaaeamaibabacaaa"
9956 DATA "aaaabodigchmaaaa","aaaiaiaiaiaibaaa"
9957 DATA "aabicecacecmdiaa","aaamaiaiaiaiaiaa"
9958 DATA "aadidgccgcecdmaa","aaaiaiaibibabiaa"
9959 DATA "aaaibmbebebmbiaa","aaaiaiaiaiaiaiaa"
9960 DATA "aaaobohgeggmdmaa","aaaiaiaiaiaiaiaa"
9961 DATA "aaambkcceceehaaa","aaaiaiaibababiaa"
9962 DATA "aabmdeaceceaceaa","aaaiaiaiaibibaaa"
9963 DATA "aaaadadmgddoaaaa","aabaaaaaaaaiaiaa"
