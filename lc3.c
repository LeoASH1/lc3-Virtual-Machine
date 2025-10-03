#include <stdio.h>
#include <stdint.h>
#include <signal.h>
/* windows only */
#include <Windows.h>
#include <conio.h>  // _kbhit



//registers
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


// conditional flags (provide info about a recently executed calculation)
enum
{
    Flag_Positive = 1 << 0,
    Flag_Zero = 1 << 1, 
    Flag_Negative = 1 << 2, 
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




//Memory Mapped Registers 8 general purpose, 2 with designated roles with special address in memory (often used for certain harware)
enum
{
    MR_KBSR = 0xFE00, // keyboard status 
    MR_KBDR = 0xFE02  // keyboard data
};

//TRAP codes PC moves to codes address of TRAP code and afterwards resets to initial instruction
enum
{
    TRAP_GETC = 0x20,  /* get character from keyboard, not echoed onto the terminal */
    TRAP_OUT = 0x21,   /* output a character */
    TRAP_PUTS = 0x22,  /* output a word string */
    TRAP_IN = 0x23,    /* get character from keyboard, echoed onto the terminal */
    TRAP_PUTSP = 0x24, /* output a byte string */
    TRAP_HALT = 0x25   /* halt the program */
};



//MEMORY STORAGE with 65,536 memory locations

// shifting 1 bit 16 places to the left
#define MEMORY_MAX (1 << 16)
// memory array with MEMORY_MAX amount of elements
uint16_t memory[MEMORY_MAX];



//Register Storage
uint16_t reg[R_COUNT];


//Input Buffering
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



//Handle Interrupt
void handle_interrupt(int signal)
{
    restore_input_buffering();
    printf("\n");
    exit(-2);
}

//Sign Extend for immediate mode as 5 bits need to be converted to 16 and the number needs to be conserved
uint16_t sign_extend(uint16_t x, int bit_count)
{
    if ((x >> (bit_count - 1)) & 1) {
        x |= (0xFFFF << bit_count);
    }
    return x;
}

//Swap
    // reads program into VM memory fixing endianess
    uint16_t swap16(uint16_t x)
    {
    return (x << 8) | (x >> 8);
    }   


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

//Read Image File (instructions get into memory of the VM by first 16 bits of program file stating address in memory where program should start) instructions of what program should do comes after
// this function opens the file, finds origin and copies over instructions
void read_image_file(FILE* file)
{
    // origin tells us where in memory to place the image 
    uint16_t origin;
    fread(&origin, sizeof(origin), 1, file);
    origin = swap16(origin);

    // figuring out the most amount of instructions we can fit in memory from the origin
    uint16_t max_read = MEMORY_MAX - origin;
    uint16_t* p = memory + origin;
    size_t read = fread(p, sizeof(uint16_t), max_read, file);

    // swap to little endian (least signifant byte read first) so its read correctly
    while (read-- > 0)
    {
        *p = swap16(*p);
        ++p;
    }
}


//Read Image which opens and closes the file whilst calling read_image_file to do the loading
int read_image(const char* image_path)
{
    FILE* file = fopen(image_path, "rb");
    if (!file) { return 0; };
    read_image_file(file);
    fclose(file);
    return 1;
}
//Memory Access uses getter(check keyboard inputs) and setter(sends information to memory address of registers for that input)  
void mem_write(uint16_t address, uint16_t val)
{
    memory[address] = val;
}

uint16_t mem_read(uint16_t address)
{
    if (address == MR_KBSR)
    {
        if (check_key())
        {
            memory[MR_KBSR] = (1 << 15);
            memory[MR_KBDR] = getchar();
        }
        else
        {
            memory[MR_KBSR] = 0;
        }
    }
    return memory[address];
}


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

    signal(SIGINT, handle_interrupt);
    disable_input_buffering();

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

            // bitwise addition so uses & instead of +
            case OP_And:
                // Destination register (store loaded value) 
                // 0x7 is hexadecimal for 7 which gives the lowest 3 bits so instruction shifts by 9 and then you keep bits [11:9]
                uint16_t register_0 = (instruction >> 9) & 0x7;
                // first operand
                uint16_t register_1 = (instruction >> 6) & 0x7;
                // for immediate mode
                uint16_t immediate_flag = (instruction >> 5) & 0x1;

                if (immediate_flag)
                {
                    uint16_t imm5 = sign_extend(instruction & 0x1F, 5);
                    reg[register_0] = reg[register_1] & imm5;   
                }

                else
                {
                    uint16_t register_2 = instruction & 0x7;
                    reg[register_0] = reg[register_1] & reg[register_2];
                }
                update_flags(register_0);

                break;

            case OP_Not:
                uint16_t register_0 = (instruction >> 9) & 0X7;
                uint16_t register_1 = (instruction >> 6) & 0x7;

                //destination register becomes the opposite of register_1
                reg[register_0] = ~reg[register_1];

                update_flags(register_0);

                break;

            case OP_Branch:
                {

                // extracts lowest 9 bits to get pc offset
                uint16_t pc_offset = sign_extend(instruction & 0x1FF, 9);

                // extracts the condition codes (negative, zero, positive)
                uint16_t cond_flag = (instruction >> 9) & 0x7;


                if (cond_flag & reg[conditional_flag])
                {

                // update PC
                reg[PC_Register] += pc_offset;
                }
                }
                
                break;
            case OP_Jump:
                {
                // sets PC to stored value in register
                uint16_t r1 = (instruction >> 6) & 0x7;
                reg[PC_Register] = reg[r1];
                }
                break;

            case OP_JumpRegister:
                {
                uint16_t long_flag = (instruction >> 11) & 1;
                reg[register_7 ] = reg[PC_Register];
                if (long_flag)
                {
                uint16_t long_pc_offset = sign_extend(instruction & 0x7FF, 11);
                reg[PC_Register] += long_pc_offset;  
                }
                else
                {
                uint16_t r1 = (instruction >> 6) & 0x7;
                reg[PC_Register] = reg[r1]; 
                }
                }

                break;
            case OP_Load:
                {
                uint16_t r0 = (instruction >> 9) & 0x7;
                uint16_t pc_offset = sign_extend(instruction & 0x1FF, 9);
                reg[r0] = mem_read(reg[PC_Register] + pc_offset);
                update_flags(r0);
                }
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
                {
                uint16_t r0 = (instruction >> 9) & 0x7;
                uint16_t r1 = (instruction >> 6) & 0x7;
                uint16_t offset = sign_extend(instruction & 0x3F, 6);
                reg[r0] = mem_read(reg[r1] + offset);
                update_flags(r0);
                }
                break;
            case OP_LEA:
            {
             uint16_t r0 = (instruction >> 9) & 0x7;
            uint16_t pc_offset = sign_extend(instruction & 0x1FF, 9);
            reg[r0] = reg[PC_Register] + pc_offset;
            update_flags(r0);
            }
                
                break;
            case OP_Store:
                {
                uint16_t r0 = (instruction >> 9) & 0x7;
                uint16_t pc_offset = sign_extend(instruction & 0x1FF, 9);
                mem_write(reg[PC_Register] + pc_offset, reg[r0]);
                }
                break;
            case OP_StoreIndirect:
               {
                uint16_t r0 = (instruction >> 9) & 0x7;
                uint16_t pc_offset = sign_extend(instruction & 0x1FF, 9);
                mem_write(mem_read(reg[PC_Register] + pc_offset), reg[r0]);
                }
                break;
            case OP_StoreRegister:
               {
                uint16_t r0 = (instruction >> 9) & 0x7;
                uint16_t r1 = (instruction >> 6) & 0x7;
                uint16_t offset = sign_extend(instruction & 0x3F, 6);
                mem_write(reg[r1] + offset, reg[r0]);
                }
                break;
            
            //Trap routines perform tasks from I/O devices such as inputs from a keyboard but arent restricted to inputs
            case OP_Trap:
                reg[register_7] = reg[PC_Register];

                switch (instruction & 0xFF)
                {
                case TRAP_GETC:
              /* read a single ASCII char */
                reg[register_0] = (uint16_t)getchar();
                update_flags(register_0);
                }
                break;
                

                case TRAP_OUT:
                {
                putc((char)reg[register_0], stdout);
                fflush(stdout);
                }
                break;
                

                // outputs a string on the console
                case TRAP_PUTS:
                {
                /* one char per word */
                uint16_t* c = memory + reg[register_0];
                while (*c)
                {
                putc((char)*c, stdout);
                ++c;
                }
                fflush(stdout);
                }
                break;

                case TRAP_IN:
                {
                printf("Enter a character: ");
                char c = getchar();
                putc(c, stdout);
                fflush(stdout);
                reg[register_0] = (uint16_t)c;
                update_flags(register_0);
                }
                break;

                case TRAP_PUTSP:
                {
                /* one char per byte (two bytes per word)
                 here we need to swap back to
                big endian format */
                uint16_t* c = memory + reg[register_0];
                while (*c)
                {
                char char1 = (*c) & 0xFF;
                putc(char1, stdout);
                char char2 = (*c) >> 8;
                if (char2) putc(char2, stdout);
                ++c;
                }
                 fflush(stdout);
                }
                break;

                case TRAP_HALT:
                {
                puts("HALT");
                fflush(stdout);
                running = 0;
                }
                break;
            case OP_Reserved:
            case OP_RTI:
            default:
                //BAD OPCODE
                break;
        }
    }
    //restores terminal settings when program inturupted 
    restore_input_buffering();
}

