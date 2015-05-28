# Onikiri

Onikiri is a cycle-accurate processor simulator. Onikiri documentation is available on <https://github.com/onikiri/onikiri2/wiki>.


## Quickstart 

1.Clone from the repository
    
    git clone https://github.com/onikiri/onikiri2 --recursive

2.Build Onikiri
  
    cd onikiri2/project/gcc/
    make
    
3.Run a sample pre-compiled alpha binary

    cd ../../benchmark/HelloWorld/
    ../../project/gcc/onikiri2/a.out param.xml
    (After that, Onikiri outputs result.xml and vis.c0.txt.gz.

4.(Visualize the pipeline log, if you are a Windows user

    Execute tool/Kanata/Kanata.exe
    Open vis.c0.txt.gz
