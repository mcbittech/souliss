/**************************************************************************
	Souliss Home Automation - Anti-theft Integration (Main Node)

	The anti-theft integration allow the handling of sensors distributed in the
	home, reporting if an alarm is raised to a standard anti-theft system.
	Two communication paths are build for the case, one care of data exchanging
	between the nodes involved in the anti-theft and the other is used to provide
	a distributed watch-dog. A fail of one node will result in a fail of the 
	watch-dog and and so rising an alarm to the anti-theft.
	
	A standard anti-theft system can be integrated via its auxiliary inputs or
	equivalent.
	
	The anti-theft can be handled remotely via the Android (or any direct) user 
	interface.
	
	Main node has:
	- Anti-theft sensor input on Pin 2,
	- Anti-theft activated output on Pin 7,
	- Anti-theft alarm output on Pin 8.
	
	Peer node has:
	- Anti-theft sensor input on Pin 2,
 
	CONFIGURATION IS MANDATORY BEFORE COMPILING
	Before compiling this code, is mandatory the configuration of the framework
	this ensure the use of proper drivers based functionalities and requested
	communication interface.	
	
	Configuration files are located on /conf folder, is suggested to use this 
	code on one of the boards listed below.	
	
	Run this code on one of the following boards:
	
		Board Conf Code			Board Model
		0x02					Freaklabs Chibiduino with Ethernet Shield		

	******************** Configuration Parameters *********************
	
		Configuration file		Parameter
		QuickCfg.h				#define	QC_ENABLE			0x01
		QuickCfg.h				#define	QC_BOARDTYPE		0x02

	Is required an additional IP configuration using the following parameters
		QuickCfg.h				const uint8_t DEFAULT_BASEIPADDRESS[] = {...}
		QuickCfg.h				const uint8_t DEFAULT_SUBMASK[]       = {...}
		QuickCfg.h				const uint8_t DEFAULT_GATEWAY[]       = {...}
	
***************************************************************************/
#include "Souliss.h"
#include "Typicals.h"
#include <SPI.h>

#define main_chibi_address		0x6510
#define main_eth_address		0x0010
#define chain_main_node			0x6510
#define chain_peer1_node		0x6511
#define chain_peer2_node		0x6512
#define network_my_subnet		0xFF00
#define network_my_supern		0x0000

#define ANTITHEFT				0			// This is the memory slot used for the execution of the anti-theft
#define WATCHDOG				1			// This is the memory slot used for the execution of the watchdog

// define the shared memory map
U8 memory_map[MaCaco_MEMMAP];

// flag 
U8 data_changed = 0;

#define time_base_fast		10				// Time cycle in milliseconds
#define time_base_slow		10000			// Time cycle in milliseconds
#define num_phases			255				// Number of phases

U8 phase_speedy=0, phase_fast=0, phase_slow=0;
unsigned long tmr_fast=0, tmr_slow=0;  

void setup()
{
	// Setup the network configuration	
	Souliss_SetAddress(main_chibi_address, network_my_subnet, network_my_supern);		

	// Setup the network configuration	
	Souliss_SetAddress(main_eth_address, network_my_subnet, network_my_supern);		  

	// Load the address also in the memory_map
	Souliss_SetLocalAddress(memory_map, main_eth_address);
	
	// Set the addresses of the remote nodes
	Souliss_SetRemoteAddress(memory_map, chain_peer1_node, 1);	
	Souliss_SetRemoteAddress(memory_map, chain_peer2_node, 2);			
	
	// Setup the anti-theft
	Souliss_SetT41(memory_map, ANTITHEFT);

	// Define inputs, outputs pins and pulldown
	pinMode(2, INPUT);									// Hardware pulldown required	
	pinMode(7, OUTPUT);		
	pinMode(8, OUTPUT);	
}

void loop()
{ 
	if(abs(millis()-tmr_fast) > time_base_fast)
	{	
		tmr_fast = millis();
		phase_fast = (phase_fast + 1) % num_phases;
				
		// Execute the code every 3 time_base_fast		  
		if (!(phase_fast % 3))
		{  
			// Input from anti-theft sensor
			Souliss_LowDigIn(2, Souliss_T4n_Alarm, memory_map, ANTITHEFT);
		
			// Execute the anti-theft logic
			Souliss_Logic_T41(memory_map, ANTITHEFT, &data_changed);
		
			// Set the Pin7 if the anti-theft is activated
			Souliss_nDigOut(7, Souliss_T4n_Antitheft, memory_map, ANTITHEFT);			
		
			// Set the Pin8 if the alarm is rised
			Souliss_LowDigOut(8, Souliss_T4n_InAlarm, memory_map, ANTITHEFT);
		}
		
		// Execute the code every 5 time_base_fast		  
		if (!(phase_fast % 5))
		{   
			// Retreive data from the communication channel
			Souliss_CommunicationData(memory_map, &data_changed);		
		}
		
		// Execute the code every 31 time_base_fast		  
		if (!(phase_fast % 31))
		{   
			// Get logic typicals once and at every refresh
			Souliss_GetTypicals(memory_map, 2);
		}
		
		// Execute the code every 51 time_base_fast		  
		if (!(phase_fast % 51))
		{   
			// Open a communication channel with remote nodes
			Souliss_CommunicationChannels(memory_map, 2);
		}	
		
		// Execute the code every 251 time_base_fast		  
		if (!(phase_fast % 251))
		{   
			// Build a watchdog chain to monitor the nodes healt
			Souliss_Watchdog(memory_map, chain_peer1_node, WATCHDOG, Souliss_T4n_Alarm);
			Souliss_LinkOI(memory_map, ANTITHEFT, WATCHDOG);									// Use the output of the watchdog as input for anti-theft
		}		
		
	}
	else if(abs(millis()-tmr_slow) > time_base_slow)
	{	
		tmr_slow = millis();
		phase_slow = (phase_slow + 1) % num_phases;
		
		// Execute the code every 7 time_base_slow		  
		if (!(phase_slow % 7))
		{
			// Refresh typical definitions
			Souliss_RefreshTypicals();
		}				
	}	
} 
