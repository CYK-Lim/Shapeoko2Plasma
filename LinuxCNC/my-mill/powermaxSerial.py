#!/usr/bin/env python

from PowermaxSerialFuncs import *
import hal
import sys


try:
	h = hal.component("powermax")
	h.newpin("minCurr", hal.HAL_FLOAT, hal.HAL_IN)
	h.newpin("maxCurr", hal.HAL_FLOAT, hal.HAL_IN)
	h.newpin("minPres", hal.HAL_FLOAT, hal.HAL_IN)
	h.newpin("maxPres", hal.HAL_FLOAT, hal.HAL_IN)
	h.newpin("runTime", hal.HAL_FLOAT, hal.HAL_IN)
	h.newpin("fltCode", hal.HAL_FLOAT, hal.HAL_IN)
	h.newpin("presFB", hal.HAL_FLOAT, hal.HAL_IN)
	h.newpin("currFB", hal.HAL_FLOAT, hal.HAL_IN)
	h.newpin("presSP", hal.HAL_FLOAT, hal.HAL_IN)
	h.newpin("currSP", hal.HAL_FLOAT, hal.HAL_IN)
	h.newpin("machON", hal.HAL_BIT, hal.HAL_IN)
	h.newpin("modeNorSP", hal.HAL_BIT, hal.HAL_IN)	
	h.newpin("modeCpaSP", hal.HAL_BIT, hal.HAL_IN)	
	h.newpin("modeGouSP", hal.HAL_BIT, hal.HAL_IN)	
	h.newpin("modeNorFB", hal.HAL_BIT, hal.HAL_OUT)	
	h.newpin("modeCpaFB", hal.HAL_BIT, hal.HAL_OUT)	
	h.newpin("modeGouFB", hal.HAL_BIT, hal.HAL_OUT)	
	h.ready()
	#print "PYTHON SERIAL: Finished creating HAL pins"
except:
	raise
	
		
h.presFB = -1
h.currFB = -1		
modeSP = 1
modeFB = 1	
machineON = False


pmax = serialinit()

if pmax is not None:
	
	while True:
		temp = readsinglereg(pmax,8344) 	#fault code
		if temp is not None:
			h.fltCode = temp
			break
	
	while True:
		temp = readsinglereg(pmax,8345) 	#min current
		if temp is not None:
			h.minCurr = temp/64
			break	
		
	while True:
		temp = readsinglereg(pmax,8346) 	#max current
		if temp is not None:
			h.maxCurr = temp/64
			break	

	while True:
		temp = readsinglereg(pmax,8348) 	#min pressure
		if temp is not None:
			h.minPres = temp/128
			break	
	
	while True:
		temp = readsinglereg(pmax,8349) 	#max pressure
		if temp is not None:
			h.maxPres = temp/128
			break	

	while True:
		temp = readsinglereg(pmax,8350) 	#low byte of time
		if temp is not None:
			templow = temp
			break	

	while True:
		temp = readsinglereg(pmax,8351) 	#high byte of time
		if temp is not None:
			temphigh = temp
			break					

	h.runTime = (temphigh<<16) | templow

	
	h.currFB = readsinglereg(pmax,8340)/64  #read current
	h.presFB = readsinglereg(pmax,8342)/128	#read pressure
	modeFB = readsinglereg(pmax,8339)		#read mode
	
	h.modeNorFB = 0
	h.modeCpaFB = 0
	h.modeGouFB = 0
	
	if modeFB == 1:
		h.modeNorFB = 1
	elif modeFB == 2:
		h.modeCpaFB = 1
	elif modeFB == 3:
		h.modeGouFB = 1

			
	try:
			
		#Main Loop
		while 1:
					
			if h.machON is True:
				if machineON is False:
		
					# these next three writes must be done 
					# together to put it in remote control
					while writereg(pmax,8339,1) is None: pass				#write mode
					while writereg(pmax,8340,(h.currSP*64)) is None: pass 	#write current
					while writereg(pmax,8342,(h.presSP*128)) is None: pass	#write pressure
					
					machineON = True
					modeSP = 1

					
				else:
					if h.modeNorSP == 1:
						modeSP = 1
					elif h.modeCpaSP == 1:
						modeSP = 2
					elif h.modeGouSP == 1:
						modeSP = 3
						
				
					if ( h.currFB != h.currSP ):
						tempCurSP = h.currSP #temp is used in case the value changes while writing
						if writereg(pmax,8340,(tempCurSP*64)) is not None: 	#write current
							h.currFB = tempCurSP								
			
					if ( h.presFB != h.presSP ):
						tempPresSP = h.presSP #temp is used in case the value changes while writing
						if writereg(pmax,8342,(tempPresSP*128)) is not None:	#write pressure
							h.presFB = tempPresSP									
			
					if ( modeFB != modeSP ):
						tempModeSP = modeSP #temp is used in case the value changes while writing
						if writereg(pmax,8339,tempModeSP) is not None: 			#write mode
							modeFB = tempModeSP
								
						h.modeNorFB = 0
						h.modeCpaFB = 0
						h.modeGouFB = 0
	
					if modeFB == 1:
						h.modeNorFB = 1
					elif modeFB == 2:
						h.modeCpaFB = 1
					elif modeFB == 3:
						h.modeGouFB = 1
								
			elif h.machON is False:
				if machineON is True:
				
					# by writing these three to zero the plasma will be
					# returned to operator control
					while writereg(pmax,8339,0) is None: pass 	#write mode to zero
					while writereg(pmax,8340,0) is None: pass	#write current to zero
					while writereg(pmax,8342,0)	is None: pass	#write pressure to zero	
					machineON = False
	
				h.currFB = -1
				h.presFB = -1
				h.modeNorFB = 0
				h.modeCpaFB = 0
				h.modeGouFB = 0
				
			h.fltCode = readsinglereg(pmax,8344)	#read fault code
					

	except KeyboardInterrupt:
		pmax.close()
		print "Closing Powermax Serial Connection"
		raise SystemExit

