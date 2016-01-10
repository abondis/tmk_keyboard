#ifndef PS2_IO_H
#define PS2_IO_H


void clock_init(void);
void clock_lo(void);
void clock_hi(void);
bool clock_in(void);

void data_init(void);
void data_lo(void);
void data_hi(void);
bool data_in(void);

void reset_init(void);
void reset_lo(void);
void reset_hi(void);
#endif
