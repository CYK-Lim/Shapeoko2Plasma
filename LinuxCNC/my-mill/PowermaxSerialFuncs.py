#!/usr/bin/env python

import pymodbus
from pymodbus.constants import Defaults
from pymodbus.exceptions import ModbusException
from pymodbus.exceptions import ConnectionException
from pymodbus.client.sync import ModbusSerialClient as ModbusClient
from pymodbus.pdu import ModbusExceptions as ecode
from pymodbus.pdu import ExceptionResponse
import pymodbus.factory
import logging


#logging.basicConfig()
#log = logging.getLogger()
#log.setLevel(logging.DEBUG)

#initialize the serial connection
def serialinit():
	try:
		client = ModbusClient(method='ascii', port='/dev/ttyMP2', parity='E', timeout=1 )
		val = client.connect()
		if val:
			print "Connection open to client."
			return client
		else:
			print "Error in Client Connection!"
			return None
	except:
		return None
	
def writereg(client, reg, num):
	try:
		result = client.write_register(reg,num)
		if isinstance( result, ExceptionResponse ):
			raise ModbusException( str( result ))
		elif result is None:
			print " *_*_*_*_*_* TIMEOUT WRITING %d over the serial *_*_*_*_*_*" %reg
			return None
		else:
			return 1
	except ModbusException as exc:
		print " *_*_*_*_*_* Error WRITING over the serial *_*_*_*_*_*"
		print str( exc ).find( "failed: Timeout" ) #"Modbus Error:" in str(exc)
		return None


def readsinglereg(client, reg):
	try:
		result = client.read_holding_registers(reg,1)
		
		if isinstance( result, ExceptionResponse ):
			raise ModbusException( str( result ))
		elif result is not None:
			return result.registers[0]
		else:
			print " *_*_*_*_*_* TIMEOUT READING %d over the serial *_*_*_*_*_*" %reg
			return None
			
	except ModbusException as exc:
		print " *_*_*_*_*_* Error READING over the serial *_*_*_*_*_*"
		print str( exc ).find( "failed: Timeout" ) #"Modbus Error:" in str(exc)
		return None
		
