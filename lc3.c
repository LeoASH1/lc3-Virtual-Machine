#include <stdio.h>
#include <stdint.h>
#include <signal.h>
/* windows only */
#include <Windows.h>
#include <conio.h>  // _kbhit


HANDLE hStdin = INVALID_HANDLE_VALUE;
DWORD fdwMode, fdwOldMode;

void disable_input_buffering()
{
    hStdin = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(hStdin, &fdwOldMode); /* save old mode */
    fdwMode = fdwOldMode
            ^ ENABLE_ECHO_INPUT  /* no input echo */
            ^ ENABLE_LINE_INPUT; /* return when one or
                                    more characters are available */
    SetConsoleMode(hStdin, fdwMode); /* set new mode */
    FlushConsoleInputBuffer(hStdin); /* clear buffer */
}

void restore_input_buffering()
{
    SetConsoleMode(hStdin, fdwOldMode);
}

uint16_t check_key()
{
    return WaitForSingleObject(hStdin, 1000) == WAIT_OBJECT_0 && _kbhit();
}


//Memory Mapped Registers 8 general purpose, 2 with designated roles
enum
{
    register_0 = 0,
    register_1,
    register_2,
    register_3,
    register_4,
    register_5,
    register_6,
    register_7,
    PC_Register, 
    conditional_flag,
    R_COUNT
};

//OPCodes (tells the computer to do a simple task)
enum
{
    OP_Branch = 0, /* branch */
    OP_Add,    /* add  */
    OP_Load,     /* load */
    OP_Store,     /* store */
    OP_JumpRegister,    /* jump register */
    OP_And,    /* bitwise and */
    OP_LoadRegister,    /* load register */
    OP_StoreRegister,    /* store register */
    OP_RTI,    /* unused */
    OP_Not,    /* bitwise not */
    OP_LoadIndirect,    /* load indirect */
    OP_StoreIndirect,    /* store indirect */
    OP_Jump,    /* jump */
    OP_Reserved,    /* reserved (unused) */
    OP_LEA,    /* load effective address */
    OP_Trap   /* execute trap */
};


//MEMORY STORAGE with 65,536 memory locations

// shifting 1 bit 16 places to the left
#define MEMORY_MAX (1 << 16)
// memory array with MEMORY_MAX amount of elements
uint16_t memory[MEMORY_MAX];


// conditional flags (provide info about a recently executed calculation)
enum
{
    Flag_Positive = 1 << 0,
    Flag_Zero = 1 << 1, 
    Flag_Negative = 1 << 2, 
};

//Register Storage
uint16_t reg[R_COUNT];

//Input Buffering
//Handle Interrupt

//Sign Extend for immediate mode as 5 bits need to be converted to 16 and the number needs to be conserved
uint16_t sign_extend(uint16_t x, int bit_count)
{
    if ((x >> (bit_count - 1)) & 1) {
        x |= (0xFFFF << bit_count);
    }
    return x;
}
//Swap
//Update Flags depenidng on Opcode result
void update_flags(uint16_t r)
{
    if (reg[r] >> 15) 
    {
        reg[conditional_flag] = Flag_Negative;
    }
    else if (reg[r] == 0)
    {
        reg[conditional_flag] = Flag_Zero;
    }
    else
    {
        reg[conditional_flag] = Flag_Positive;
    }
}

//Read Image File
//Read Image
//Memory Access



//main loop initilising argument count and argument vector
int main(int argc, const char* argv[])
{
    // Load the arguments and set up here, making sure a program name and image(binary snapshot of program) are provided
    if (argc < 2)
    {
       printf("lc3 [image-file1] ...\n");
       exit(2);
    }

    for (int j = 1; j < argc; ++j)
    {
        // if you read the file and it does not have needed info:
        if (!read_image(argv[j]))
        {
            printf("failed to load image: %s\n", argv[j]);
            exit(1);
        }
    }

    // initialise a zero flag as one should be set at any time
    reg[conditional_flag] = Flag_Zero;

    // start the position of the PC
    enum { PC_START = 0x3000 };
    reg[PC_Register] = PC_START;




    
    // starting while loop until running stops being 0
    int running = 1;
    while (running)
    {
    uint16_t instruction = mem_read(reg[PC_Register]++); 
    uint16_t opcode = instruction >> 12;

    // list of opp codes
    switch (opcode)
        {
            case OP_Add:
                // Destination register (store loaded value)
                uint16_t register_0 = (instruction >> 9) & 0x7;
                // first operand
                uint16_t register_1 = (instruction >> 6) & 0x7;
                // for immediate mode
                uint16_t immediate_flag = (instruction >> 5) & 0x1;

                if (immediate_flag)
                {
                    uint16_t imm5 = sign_extend(instruction & 0x1F, 5);
                    reg[register_0] = reg[register_1] + imm5;
                }

                else
                {
                    uint16_t register_2 = instruction & 0x7;
                    reg[register_0] = reg[register_1] + reg[register_2];
                }

                update_flags(register_0);
                break;
            case OP_And:
                
                break;
            case OP_Not:
                
                break;
            case OP_Branch:
                
                break;
            case OP_Jump:
                
                break;
            case OP_JumpRegister:
                
                break;
            case OP_Load:
                
                break;

            //loads a value from memory to register, its encoding contains Opcode 1010 and oprands a destination register and PCoffset9 which is a value imbedded in instruction (address for number in memory)
            case OP_LoadIndirect:
               {
                // gets the destination register
                uint16_t register_0 = (instruction >> 9) & 0x7;
                // gets PC offset bits: 8 to 0
                uint16_t pc_offset = sign_extend(instruction & 0x1FF, 9);
                // adds PC offset to the current PC, Then gets memory location of final address 
                reg[register_0] = mem_read(mem_read(reg[PC_Register] + pc_offset));
                update_flags(register_0);
}
                break;
            case OP_LoadRegister:
                
                break;
            case OP_LEA:
                
                break;
            case OP_Store:
                
                break;
            case OP_StoreIndirect:
               
                break;
            case OP_StoreRegister:
               
                break;
            case OP_Trap:
                
                break;
            case OP_Reserved:
            case OP_RTI:
            default:
                //BAD OPCODE
                break;
        }
    }
    //Shutdown
}

