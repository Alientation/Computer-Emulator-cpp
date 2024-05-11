#include "emulator32bit/Memory.h"
#include "assert.h"
#include "iostream"

Memory::Memory(word mem_size, word lo_addr, word hi_addr) {
	assert(lo_addr >= 0 && hi_addr >= lo_addr);
	assert(mem_size == hi_addr - lo_addr + 1);

	this->mem_size = mem_size;
	this->data = new byte[mem_size];

	this->lo_addr = lo_addr;
	this->hi_addr = hi_addr;
}

Memory::~Memory() {
	delete[] this->data;
}

bool Memory::in_bounds(word address) {
	return address >= this->lo_addr && address <= this->hi_addr;
}

word Memory::read(word address, MemoryReadException &exception, int num_bytes) {
	if (!in_bounds(address) || !in_bounds(address + num_bytes - 1)) {
		exception.type = MemoryReadException::Type::OUT_OF_BOUNDS_ADDRESS;
		exception.address = address;
		return 0;
	}

	word value = 0;
	for (int i = 0; i < num_bytes; i++) {
		value <<= 8;
		value += this->data[(word) (address + i)];
	}
	return value;
}

void Memory::write(word address, word value, MemoryWriteException &exception, int num_bytes) {
	if (!in_bounds(address) || !in_bounds(address + num_bytes - 1)) {
		exception.type = MemoryWriteException::Type::OUT_OF_BOUNDS_ADDRESS;
		exception.address = address;
		exception.value = value;
		exception.num_bytes = num_bytes;
		return;
	}

	for (int i = 0; i < num_bytes; i++) {
		this->data[(word) (address + i)] = value & 0xFF;
		value >>= 8;
	}
}


byte Memory::readByte(word address, MemoryReadException &exception) {
	return this->read(address, exception, 1);
}

hword Memory::readHalfWord(word address, MemoryReadException &exception) {
	return this->read(address, exception, 2);
}

word Memory::readWord(word address, MemoryReadException &exception) {
	return this->read(address, exception, 4);
}

void Memory::writeByte(word address, byte data, MemoryWriteException &exception) {
	this->write(address, data, exception);
}

void Memory::writeHalfWord(word address, hword data, MemoryWriteException &exception) {
	this->write(address, data, exception);
}

void Memory::writeWord(word address, word data, MemoryWriteException &exception) {
	this->write(address, data, exception);
}


/*
	RAM
*/
RAM::RAM(word mem_size, word lo_addr, word hi_addr) : Memory(mem_size, lo_addr, hi_addr) {}


/*
	ROM
*/

ROM::ROM(byte (&rom_data)[], word lo_addr, word hi_addr) : Memory(hi_addr - lo_addr + 1, lo_addr, hi_addr) {
	for (word i = 0; i < mem_size; i++) {
		this->data[i] = rom_data[i];
	}
}

void ROM::write(word address, word value, MemoryWriteException &exception, int num_bytes) {
	exception.type = MemoryWriteException::Type::ACCESS_DENIED;
	exception.address = address;
	exception.value = value;
	exception.num_bytes = num_bytes;
}