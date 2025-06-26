#include <iostream>
#include <cstring> //strcmp es un comparador lexicográfico, no un bool
//#include "LRU.h"
#include "manual_LRU.h"

int main(){ //sin argc, argv, in console-behavior;
    
    //LRUBufferManager buffer(3); //3 frames
    BufferPool buffer(4);

    //Buffer_Pool 
    int n=0; //time simulator

    char command[4]; //Formato "Bloque, Operacion, Pin Status", "\0"
    scanf("%3s" , command);
    while(std::strcmp(command,"!q")!= 0){
        if(command[1]-'0' < 0 || command[1]-'0' > 1 || command[2]-'0' < 0 || command[2]-'0' > 1){
            std::cout << "Operacion No Reconocida: " << command << std::endl;
            scanf("%3s", command);
            n++;
            continue;
        }

   //     buffer.access(command[0], n, command[1]-'0');
        buffer.access(command[0],command[1]-'0',command[2]-'0');
        buffer.printFrames();
        buffer.printStats();
        n++;
        scanf("%3s", command);        
    }
    std::cout << "Ended: " << std::endl;

    return 0;
}