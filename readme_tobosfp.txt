 1. Load ToBoSFP
 2. Put Cassete with train_basic_tobos.tap
 3. Load source code with: 
      LOAD ""  
 4. For "fast" training comment line 36
 5. Set destructive mode of compiler: 
      POKE 53240,0 
 6. Set short errors for compiler:     
      POKE 53252,0
 7. Build Program
      RANDOMIZE USR 53100
 9. Run program with (written on screen)
      RANDOMIZE USR 24000
10. You will be asked to load train data from tape
11. Once finished test data need to be loaded

