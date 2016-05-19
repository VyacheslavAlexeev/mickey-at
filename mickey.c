/************************************************* Developer: Pratyay Mukherjee
email: pratyay85@gmail.com ***************************************************/
#include <stdio.h>

typedef unsigned char uchar;
/*Constants as defined by the algorithm are converted into masks that can be directly \texttt{XOR}-ed when required*/
uchar rtaps[13] = {0xde, 0x4c, 0x9e, 0x48, 0x6, 0x66, 0x2a, 0xad, 0xf1, 0x81, 0xe1, 0xfb, 0xc0};
uchar comp0[13] = {0xc, 0x5e, 0x95, 0x56, 0x90, 0x15, 0x42, 0x9e, 0x57, 0xfd, 0x7e, 0xa0, 0x60};
uchar comp1[13] = {0x59, 0x79, 0x46, 0xbb, 0xc6, 0xb8, 0x45, 0xc7, 0xeb, 0xbc, 0x43, 0x89, 0x80};
uchar fb0[13] = {0xf5, 0xfe, 0x5f, 0xf9, 0x81, 0xc9, 0x52, 0xf5, 0x40, 0x1a, 0x37, 0x39, 0x80};
uchar fb1[13] = {0xee, 0x1d, 0x31, 0x32, 0xc6, 0xd, 0x88, 0x92, 0xd4, 0xa3, 0xdf, 0x2, 0x10};

/* Test set #1*/
uchar Key[10] = {0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0, 0x12, 0x34}; 
uchar IV[4] = {0x21, 0x43, 0x65, 0x87};
uchar IVlength = 4;

uchar KeyStream[16];

struct mickey_registers {
  uchar R[13];
  uchar S[13];
};

typedef struct mickey_registers mickey;

/*Clocking the register R*/
void clock_r(mickey *m, uchar Input_Bit_R, uchar Control_Bit_R) {
  uchar Feedback_Bit, i; 
  uchar carry_bits[13];

  Feedback_Bit = ((m->R[12] & 16)>>4) ^ Input_Bit_R;

  /*Carry bits are required to perform right shift across successive variables of the character array*/
  carry_bits[0] = 0; 
  for (i=0 ; i<12 ; i++)
    carry_bits[i+1] = (m->R[i] & 1)<<7;

  if (Control_Bit_R)
    for (i=0 ; i<13 ; i++)
      m->R[i] ^= (m->R[i]>>1) ^ carry_bits[i];
  else
    for (i=0 ; i<13 ; i++)
      m->R[i] = (m->R[i]>>1) ^ carry_bits[i];

  if (Feedback_Bit)
    for (i=0 ; i<13 ; i++)
      m->R[i] ^= rtaps[i];
}

void clock_s(mickey *m, uchar Input_Bit_S, uchar Control_Bit_S) {
  uchar Feedback_Bit, i; 
  uchar carry_bits_right[13]; 
  uchar carry_bits_left[13]; 
  uchar temp;

  Feedback_Bit = ((m->S[12] & 16)>>4) ^ Input_Bit_S;
  /*Carry bits are required to perform right and left shifts across successive variables of the character array*/
  carry_bits_right[0] = 0;
  for (i=0 ; i<12; i++)
    carry_bits_right[i+1] = (m->S[i] & 1)<<7; 
  carry_bits_left[12] = 0;
  for (i=1 ; i<13 ; i++)
    carry_bits_left[i-1] = (m->S[i] & 128)>>7;

  /*Generating "S hat"*/
  temp = (m->S[12]>>1) & 0x10; 
  for (i=0 ; i<13 ; i++)
    m->S[i] = ((m->S[i]>>1) ^ carry_bits_right[i]) ^ ((m->S[i] ^ comp0[i]) & (m->S[i]<<1 ^ carry_bits_left[i] ^ comp1[i]));

  /* 0th bit of S-hat = 0 */
  m->S[0] &= 0x7f;
  /* 99th bit of S-hat = 98th bit of S */
  m->S[12] &= 0xef;
  m->S[12] ^= temp;
  if (Feedback_Bit) {
    if (Control_Bit_S) {
      for (i=0 ; i<13 ; i++)
        m->S[i] ^= fb1[i];
    }
    else
    {
      for (i=0; i<13; i++)
        m->S[i] ^= fb0[i];
    } 
  }
}


void clock_kg(mickey *m, uchar Mixing, uchar Input_Bit) {
  uchar Control_Bit_R, Control_Bit_S, Input_Bit_R, Input_Bit_S;

  Control_Bit_R = ((m->S[4] & 32)>>5) ^ ((m->R[8] & 16)>>4); 
  Control_Bit_S = ((m->S[8] & 16)>>4) ^ ((m->R[4] & 64)>>6);

  if (Mixing)
    Input_Bit_R = Input_Bit ^ ((m->S[6] & 32)>>5);
  else
    Input_Bit_R = Input_Bit; 
  Input_Bit_S = Input_Bit;

  clock_r(m, Input_Bit_R, Control_Bit_R); 
  clock_s(m, Input_Bit_S, Control_Bit_S);
}

void print_keystream() {
  int i;
  printf("\nKey Stream\n"); 

  for (i=0 ; i<16 ; i++)
    printf("%x ",KeyStream[i]); printf("\n");
}

int main() {
  mickey m;
  uchar i, j, Input_Bit;

  /*Initialise*/
  for (i=0 ; i<13 ; i++) {
    m.S[i] = 0;
    m.R[i] = 0;
  }

  /*Load IV*/
  int counter = 0;
  for (i=0 ; i<IVlength ; i++) {
    for (j=0 ; j<8 ; j++) {
      Input_Bit = (IV[i]>>(7-j)) & 1;
      clock_kg(&m, 1, Input_Bit);
      counter++;
    }
  }

    /*Load Key*/
  for (i=0 ; i<10 ; i++) {
    for (j=0 ; j<8 ; j++) {
      Input_Bit = (Key[i]>>(7-j)) & 1;
      clock_kg(&m , 1, 0);
    }
  }

  /*Preclock*/
  for (i=0 ; i<100 ; i++) 
    clock_kg(&m , 1, 0);

  /*Generate Key Stream*/
  for (i=0 ; i<16 ; i++) {
    KeyStream[i] = 0; 
    for (j=0 ; j<8 ; j++) {
      KeyStream[i] ^= ((m.R[0] ^ m.S[0]) & 128)>>j; 
      clock_kg(&m, 0, 0);
    } 
  }
  /*Print KeyStream*/
  print_keystream();

  return 0;
}
