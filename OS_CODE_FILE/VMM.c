#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <alloca.h>
#include <stdbool.h>

// function headers
void getPage(int logicaladdress,int choice);
int backingStore(int pageNum);
void TLBInsert(int pageNum, int frameNum);

// define filetypes for files
FILE    *addresses;
FILE    *backing_store;

#define BUFF_SIZE 10                   // buffer size for reading a line from addresses file
#define ADDRESS_MASK 0xFFFF            // for masking all of logical_address except the address
#define OFFSET_MASK 0xFF               // for masking the offset
#define TLB_SIZE 16                    // 16 entries in the TLB
#define PAGE_TABLE_SIZE 256            // page table of size 2^8
#define PAGE 256                       // upon page fault, read in 256-byte page from BACKING_STORE
#define FRAME_SIZE 256                 // size of each frame


int TLBEntries = 0;                    // current number of TLB entries
int hits = 0;                          // counter for TLB hits
int pagehit=0;                         // counter for page hits
int faults = 0;                        // counter for page faults
int currentPage = 0;                   // current number of pages
int logical_address;                   // int to store logical address
int TLBpages[TLB_SIZE];                // array to hold page numbers in TLB
int TLBFrequency[TLB_SIZE];		//array to store the occurance of each page in TLB
_Bool pagesRef[PAGE_TABLE_SIZE];        // array to hold reference bits for page numbers in TLB
int pageTableNumbers[PAGE_TABLE_SIZE]; // array to hold page numbers in page table
char currentAddress[BUFF_SIZE];        // array to store addresses
unsigned char fromBackingStore[PAGE];    // holds reads from BACKING_STORE
unsigned char byte;                      // holds value of physical memory at frame number/offset
int physicalMemory[PAGE_TABLE_SIZE][FRAME_SIZE];          // physical memory array of 32,678 bytes (128 frames x 256-byte frame size)

// main opens necessary files and calls on getPage for every entry in the addresses file
int main(int argc, char *argv[])
{
    // error checking for arguments
    /*if (argc != 2)
    {
        fprintf(stderr,"Usage: ./a.out [input file]\n");
        return -1;
    } // end if
    */
    // set pagesRef array to 0
for(int i = 0; i < 16; i++)
    {
      TLBFrequency[i] = 0;
    } // end for

int i;
for(i = 0; i < 128; i++)
    {
      pagesRef[i] = 0;
    } // end for

    // open backing store file
    backing_store = fopen("BACKING_STORE.bin", "rb");

    // error checking for opening file
    if (backing_store == NULL)
    {
        fprintf(stderr, "Error opening BACKING_STORE.bin%s\n","BACKING_STORE.bin");
        return -1;
    } // end if

    // open virtual addresses file
    addresses = fopen("addresses.txt", "r");

    // error checking for opening file
    if (addresses == NULL)
    {
        fprintf(stderr, "Error opening addresses.txt %s\n",argv[1]);
        return -1;
    } // end if

    // define number of translated addresses
    int numberOfTranslatedAddresses = 0;
int choice =0;
printf("\nEnter the replacement algorithm you want to apply\n");
printf("1.FIFO\n2.LIFO\n3.MFU\n4.LFU\n");
scanf("%d",&choice);
 
if(choice > 4){
	printf("Please enter a valid Choice. Exiting the program\n");
	return 0;
}

	while ( fgets(currentAddress, BUFF_SIZE, addresses) != NULL)
    {
        logical_address = atoi(currentAddress);

        // get the physical address and byte stored at that address
        getPage(logical_address,choice);
        numberOfTranslatedAddresses++;  // increment the number of translated addresses
    } // end while
	
    // calculate and print out the stats
    double pfRate = faults / (double)numberOfTranslatedAddresses;
    double TLBRate = hits / (double)numberOfTranslatedAddresses;
    double pageRate = pagehit / (double)numberOfTranslatedAddresses;

    printf("\n\t****************Summary****************\n");
    printf("\n\t\tPage Faults = %d\n", faults);
    printf("\t\tPage Fault Rate = %.3f\n",pfRate);
    printf("\t\tTLB Hits = %d\n", hits);
    printf("\t\tTLB Hit Rate = %.3f\n", TLBRate);
    printf("\t\tPage Table Hits = %d\n", pagehit);
    printf("\t\tPage Table Hit Rate = %.3f\n", pageRate);
    printf("\n\t******************END******************\n");
  float m;
//printf("%.3f",pageRate+0.1);
//float p= pageRate;
//printf("%f",p);
printf("\n");
printf("Page Faults Rate: |");
float k;
for(k=0.0; k< pfRate; k=k+.01)
{ printf("-");
}
printf("\n");

printf("TLB Hits:  \t  |");
float l;
for(l=0.0; l< TLBRate; l=l+.01)
{ printf("-");
}
printf("\n");

printf("Page Table Hits : |");
for(m=0.0; m< pageRate;m=m+.01)
{ printf("-");
}
printf("\n\n");
    // close files
    fclose(addresses);
    fclose(backing_store);

    return 0;
} // end main
// function to read from backing store
int backingStore(int pageNum)
{

  int counter = 0;

  // position to read from pageNum
  // SEEK_SET reads from beginning of file
  if (fseek(backing_store, pageNum * PAGE, SEEK_SET) != 0)
  {
    fprintf(stderr, "Error seeking in backing store\n");
  } // end if


  if (fread(fromBackingStore, sizeof(signed char), PAGE, backing_store) == 0)
  {
    fprintf(stderr, "Error reading from backing store\n");
  } // end if

  // boolean for while loop
  _Bool search = true;

  // second chance algorithm
  while(search)
  {
    if(currentPage == PAGE_TABLE_SIZE)
    {
      currentPage = 0;
    } // end if

    // if reference bit is 0
    if(pagesRef[currentPage] == false)
    {
      // replace page
      pageTableNumbers[currentPage] = pageNum;

      // set search to false to end loop
      search = false;
    } // end if
    // else if reference bit is 1
    else
    {
      // set reference bit to 0
      pagesRef[currentPage] = false;
    } // end else
    currentPage++;
  } // end while
  // load contents into physical memory
  int i;
  for(i = 0; i < PAGE; i++)
  {
    physicalMemory[currentPage-1][i] = fromBackingStore[i];

  } // end for
  counter = currentPage-1;

  return counter;
} // end backingStore

// insert page into TLB
void TLBInsertFIFO(int pageNum, int frameNum){

    int i;  // search for entry in TLB
    for(i = 0; i < TLBEntries; i++)
    {
        if(TLBpages[i] == pageNum)
        {
            break; // break if entry found
        } // end if
    } // end for

    // if the number of entries is equal to the index
    if(i == TLBEntries)
    {
        // if TLB is not full
        if(TLBEntries < TLB_SIZE)
        {
            // insert page with FIFO replacement
            TLBpages[TLBEntries] = pageNum;
        } // end if
        // else, TLB is full
        else
        {
            // shift everything over
            for(i = 0; i < TLB_SIZE - 1; i++)
            {
                TLBpages[i] = TLBpages[i + 1];
            } // end for

            //FIFO replacement
            TLBpages[TLBEntries-1] = pageNum;

        } // end else
    } // end if

    // if the number of entries is not equal to the index
    else
    {
        // move everything over up to the number of entries - 1
        for(i = i; i < TLBEntries - 1; i++)
        {
            TLBpages[i] = TLBpages[i + 1];
        } // end for

        // if still room in TLB
        if(TLBEntries < TLB_SIZE)
        {
            // insert page at the end
            TLBpages[TLBEntries] = pageNum;

        } // end if
        // else if TLB is full
        else
        {
            // place page at number of entries - 1
            TLBpages[TLBEntries-1] = pageNum;

        } // end else
    } // end else

    // if TLB is still not full, increment the number of entries
    if(TLBEntries < TLB_SIZE)
    {
        TLBEntries++;
    } // end if
printf("***********************************");
     printf("\nTLB has been updated with Page Number %d and Frame Number %d\n\n",pageNum,frameNum);
printf("***********************************");
} // end TLBInsertFIFO


void TLBInsertLFU(int pageNum, int frameNum){

    int i;  // search for entry in TLB
    for(i = 0; i < TLBEntries; i++)
    {
        if(TLBpages[i] == pageNum)
        {
            break; // break if entry found
        } // end if
    } // end for

    // if the number of entries is equal to the index
    if(i == TLBEntries)
    {
        // if TLB is not full
        if(TLBEntries < TLB_SIZE)
        {
            // insert page with FIFO replacement
            TLBpages[TLBEntries] = pageNum;
        } // end if
        // else, TLB is full
        else
        {
		int min= TLBFrequency[0];
            // shift everything over
            for(i = 1; i < TLB_SIZE - 1; i++)
            {
		
		if(TLBFrequency[i] < min){
		min= TLBFrequency[i];
		}
                
            } // end for

            //LFU replacement
            TLBpages[TLBFrequency[min]] = pageNum;

        } // end else
    } // end if

    

    // if TLB is still not full, increment the number of entries
    if(TLBEntries < TLB_SIZE)
    {
        TLBEntries++;
    } // end if
printf("***********************************");
     printf("\nTLB has been updated with Page Number %d and Frame Number %d\n\n",pageNum,frameNum);
printf("***********************************");
} // end TLBInsertLFU

void TLBInsertMFU(int pageNum, int frameNum){

    int i=0;  // search for entry in TLB
    while(i < TLBEntries)
    {
        if(TLBpages[i] == pageNum)
        {
            break; // break if entry found
        } // end if
	i++;
    } //end while

   if(i == TLBEntries){
        // if TLB is not full
        if(TLBEntries < TLB_SIZE)
        {
            // insert page with MFU replacement
            TLBpages[TLBEntries] = pageNum;
        } // end if
        // else, TLB is full
        else
        {
		int max= TLBFrequency[0];
            // shift everything over
            for(i = 1; i < TLB_SIZE-1; i++)
            {
		
		if(TLBFrequency[i] > max){
		max= TLBFrequency[i];
		}
                
            } // end for

            //LFU replacement
            TLBpages[TLBFrequency[max]] = pageNum;

        } // end else
    }

    

    // if TLB is still not full, increment the number of entries
    if(TLBEntries < TLB_SIZE)
    {
        TLBEntries++;
    } // end if
printf("***********************************");
     printf("\nTLB has been updated with Page Number %d and Frame Number %d\n\n",pageNum,frameNum);
printf("***********************************");
} // end TLBInsertMFU


void TLBInsertLIFO(int pageNum, int frameNum){

    int i;  // search for entry in TLB
    for(i = 0; i < TLBEntries; i++)
    {
        if(TLBpages[i] == pageNum)
        {
            break; // break if entry found
        } // end if
    } // end for
	if(TLBEntries< TLB_SIZE)
		
            TLBpages[TLBEntries] = pageNum;
        else
		TLBpages[TLB_SIZE-1] = pageNum;
      

    // if TLB is still not full, increment the number of entries
    if(TLBEntries < TLB_SIZE)
    {
        TLBEntries++;
    } // end if
printf("***********************************");
     printf("\nTLB has been updated with Page Number %d and Frame Number %d\n\n",pageNum,frameNum);
printf("***********************************");
} // end TLBInsertLIFO


// function to take the logical address and obtain the physical address and byte stored at that address

void getPage(int logical_address,int choice){

    // initialize frameNum to -1, sentinel value
    int frameNum = -1;

    // mask leftmost 16 bits, then shift right 8 bits to extract page number
    int pageNum = ((logical_address & ADDRESS_MASK)>>8);

    // offset is just the rightmost bits
    int offset = (logical_address & OFFSET_MASK);

    // look through TLB
    int i;
    for(i = 0; i < TLB_SIZE; i++)
    {
      // if TLB hit
      if(TLBpages[i] == pageNum)
      {
        // extract frame number
        frameNum = i;
printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");
         printf("\nPage found in TLB\n");

        // increase number of hits
        hits++;
	TLBFrequency[frameNum]++;

      } // end if
    } // end for

    // if the frame number was not found in the TLB
    if(frameNum == -1)
    {
      int i;
      for(i = 0; i < currentPage; i++)
      {
        // if page number found in page table, extract it
        if(pageTableNumbers[i] == pageNum)
        {

            pagehit++;
          frameNum = i;

          // change reference bit
          pagesRef[i] = true;
printf("***********************************");
          printf("\nPage found in Page Table\n");

        } // end if
      } // end for
    } // end if
      // if frame number is still -1, pageNum has not been found in TLB or page table

     if(frameNum == -1)
      {
        // read from BACKING_STORE.bin
        int count = backingStore(pageNum);

        // increase the number of page faults
        faults++;

        // change frame number to first available frame number
        frameNum = count;

 switch(choice){
 case 1:

    // insert page number and frame number into TLB
        TLBInsertFIFO(pageNum, frameNum);
	break;
      
case 2:

    // insert page number and frame number into TLB
        TLBInsertLIFO(pageNum, frameNum);

      break;

case 3:

    // insert page number and frame number into TLB
        TLBInsertMFU(pageNum, frameNum);
	break;

case 4:

    // insert page number and frame number into TLB
        TLBInsertLFU(pageNum, frameNum);
	break;
    
}//end switch
 } // end if
    // assign the value of the signed char to byte
    byte = physicalMemory[frameNum][offset];


    // output the virtual address, physical address and byte of the signed char to the console

    printf("Virtual address: %d \nPhysical address: %d\nFrame Number:  %d\nOffset: %d\n", logical_address, (frameNum << 8) | offset, frameNum,offset);
} // end getPage
