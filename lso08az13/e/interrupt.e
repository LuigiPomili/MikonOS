#ifndef INTR_E 
#define INTR_E

extern void ints_handler(void);

extern memaddr getSigPseudo();

extern void setSigPseudo(memaddr value);

extern unsigned int getSigDevice();

extern void setSigDevice(unsigned int value);

extern unsigned int getSigTerminal();

extern void setSigTerminal(unsigned int value);

#endif
