#include <SPI.h>
#include <SD.h>

/*
 * Port 10h      Latch(write-only)
 *      11h     DATA(read-only)
 *      12h     Pooling(read-only)
 *      13h     Write protection status(read-only)
 */

#define TRK_NUM    40
#define SEC_NUM     16
#define SECSIZE_VZ  154
#define TRKSIZE_VZ  SECSIZE_VZ * SEC_NUM

//#define DEBUG_TRACK 1

#define TRKSIZE_FM  3172

typedef struct SectorHeader {
  uint8_t   GAP1[7];
  uint8_t   IDAM_leading[4];
  uint8_t   TR;
  uint8_t   SC;
  uint8_t   TS_sum;
  uint8_t   GAP2[6];
  uint8_t   IDAM_closing[4];
} sec_hdr_t;

typedef struct Sector {
  sec_hdr_t   header;
  uint8_t     data_bytes[126];
  uint8_t     next_track;
  uint8_t     next_sector;
  uint16_t    checksum;
} sector_t;

typedef struct Track {
  sector_t    sector[16];
} track_t;

uint8_t   fdc_data[TRKSIZE_VZ];
uint8_t   fm_track_data[TRKSIZE_FM];

char str_buf[100]={ 0 };


int get_track(File f, int n)
{
  int size = TRKSIZE_VZ;
  int offset = TRKSIZE_VZ * n;

  if (n<0 || n>39)
  {
    sprintf(str_buf, "Invalid track number %d", offset);
    Serial.println(str_buf);
    return -1;
  }

  if (f == false)
  {
    Serial.println("DSK File is not opened");
    return -1;
  }

  if (f.seek(offset) == false)
  {
    sprintf(str_buf, "Failed seek file to %d", offset);
    Serial.println(str_buf);
    return -1;
  }

  //Read track
  sprintf(str_buf, "Read track #%d", n);
  Serial.println(str_buf);
    
  if (f.read(fdc_data, size) == -1)
  {
    sprintf(str_buf, "Failed to read track %d", n);
    Serial.println(str_buf);
    return -1;
  }

  Serial.println("Track 0 data is ready\r\n");  

#ifdef  DEBUG_TRACK
  for(int i=0; i<SEC_NUM; i++)
  {
    sector_t *sec = (sector_t *)&fdc_data[i* sizeof(sector_t)];
    sec_hdr_t *hdr = &sec->header;

    sprintf(str_buf, "\r\nSector # %02x", i);
    Serial.println(str_buf);
    
    Serial.print("GAP1: ");
    for(int j=0; j<sizeof(hdr->GAP1); j++)
    {
      sprintf(str_buf, "%02X ", hdr->GAP1[j]);
      Serial.print(str_buf);
    }
 
    Serial.print("\r\nIDAM_leading: ");
    for(int j=0; j<sizeof(hdr->IDAM_leading); j++)
    {
      sprintf(str_buf, "%02X ", hdr->IDAM_leading[j]);
      Serial.print(str_buf);
    }

    sprintf(str_buf, "\r\nTR: %02X, SC: %02X, T+S: %02X", hdr->TR, hdr->SC, hdr->TS_sum);
    Serial.println(str_buf);

    Serial.print("GAP2: ");
    for(int j=0; j<sizeof(hdr->GAP2); j++)
    {
      sprintf(str_buf, "%02X ", hdr->GAP2[j]);
      Serial.print(str_buf);
    }

    Serial.print("\r\nIDAM_closing: ");
    for(int j=0; j<sizeof(hdr->IDAM_closing); j++)
    {
      sprintf(str_buf, "%02X ", hdr->IDAM_closing[j]);
      Serial.print(str_buf);
    }

    sprintf(str_buf, "\r\nNext TR: %02X, Next SC: %02X", sec->next_track, sec->next_sector);
    Serial.println(str_buf);

  }
#endif    
}

//Good values
#if 1
#define delay_1us   __builtin_avr_delay_cycles (14)
#define delay_11us  __builtin_avr_delay_cycles (170)
#define delay_20us  __builtin_avr_delay_cycles (300)
#define delay_31_2us  __builtin_avr_delay_cycles (485)
#else
//Testing values
#define delay_1us   __builtin_avr_delay_cycles (15)
#define delay_11us  __builtin_avr_delay_cycles (190)
#define delay_20us  __builtin_avr_delay_cycles (290)
#define delay_31_2us  __builtin_avr_delay_cycles (520)
#endif

#define RD_HIGH   PORTB |= 0x80;
#define RD_LOW    PORTB &= ~0x80;

inline void put_byte(byte b)
{
  byte mask[8] = { 0xf0, 0x40, 0x20, 0x10, 0x0f, 0x04, 0x02, 0x01 };  
  for (int i=0; i<8; i++)
  {
    //Clock bit
    RD_HIGH; delay_1us; RD_LOW;

    //Data bit
    if (b & mask[i])
    {
      delay_11us; 
      RD_HIGH; delay_1us; RD_LOW;
      delay_20us;
    }
    else
    {
      delay_31_2us; 
    }       
  }  
}

int sector_interleave[SEC_NUM] = { 0, 11, 6, 1, 12, 7, 2, 13, 8, 3, 14, 9, 4, 15, 10, 5 };
void put_track()
{
  //for(int n=0; n<TRKSIZE_VZ; n++)
  //for(int i=0; i<SEC_NUM; i++)
  int i=13;
  {
    sector_t *sec = (sector_t *)&fdc_data[i/*sector_interleave[i]*/ * sizeof(sector_t)];
    
#if 0
    sec_hdr_t *hdr = &sec->header;
    
    sprintf(str_buf, "\r\nTrk# %02X, Sect# %02X", hdr->TR, hdr->SC);
    Serial.println(str_buf);

    sprintf(str_buf, "Output sector# %d, length: %d", hdr->SC, SECSIZE_VZ);
    Serial.println(str_buf);

    Serial.println("Sec data:");
#endif
    for(int j=0; j<SECSIZE_VZ; j++)
    {
      byte v = ((byte *)sec)[j];
      //byte v = fdc_data[n];
      put_byte( v );
            
#if 0     
      if (j%32 == 0)
      {
        Serial.println("");
      }
      sprintf(str_buf, "%02X ", v );
      Serial.print(str_buf);
#endif
    }
#if 0
    Serial.println("");
#endif
  }
}

void setup() {

  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  if (!SD.begin())
  {
    Serial.println("Failed to begin on SD");
  }

  char filename[] = "FLOPPY1.DSK";
  if (!SD.exists(filename))
  {
    Serial.println("Can't find FLOPPY1.DSK");
  }
  
  File file = SD.open(filename, FILE_READ);

  if (!file)
  {
    Serial.println("Failed to open FLOPPY1.DSK file");
    while(1);
  }

  get_track(file, 0);
  file.close();

  //Pin 13 -> rd data (Output)
  DDRB = B10000000;

  //Clear rd data
  RD_LOW;

  //Pin 0 -> /En 1 (Input)
  //Pin 1 -> /Wr req (Input)  
  DDRD = B00000000;
}

void loop() {
    //Begining of track 98.8 us
    RD_HIGH;
    delay(98);
  
    put_track();    
}
