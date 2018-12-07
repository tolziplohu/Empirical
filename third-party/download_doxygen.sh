if [ -d doxygen ]; then                                                         
        echo "doxygen directory exists!"                                        
else                                                                            
        curl -O -L https://sourceforge.net/projects/doxygen/files/rel-1.8.11/doxygen-1.8.11.src.tar.gz/download
        tar -xvf doxygen-1.8.11.src.tar.gz  
        rm doxygen-1.8.11.src.tar.gz                                            
        mv doxygen-1.8.11 doxygen;                                              
fi
