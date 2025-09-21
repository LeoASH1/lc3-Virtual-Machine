//Memory Mapped Registers 8 general purpose, 2 with designated roles
enum
{
    Register_0 = 0,
    Register_1,
    Register_2,
    Register_3,
    Register_4,
    Register_5,
    Register_6,
    Register_7,
    Register_PC, 
    Register_ConditionalFlags,
    R_COUNT
};

//TRAP Codes



//MEMORY STORAGE with 65,536 memory locations

// shifting 1 bit 16 places to the left
#define MEMORY_MAX (1 << 16)
// memory array with MEMORY_MAX amount of elements
uint16_t memory[MEMORY_MAX];  




//Register Storage
uint16_t reg[R_COUNT];

//Input Buffering
//Handle Interrupt
//Sign Extend
//Swap
//Update Flags
//Read Image File
//Read Image
//Memory Access

//Main Loop