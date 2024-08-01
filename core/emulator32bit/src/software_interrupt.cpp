#include "emulator32bit/emulator32bit.h"
#include "util/loggerv2.h"

#define UNUSED(x) (void)(x)

void Emulator32bit::_emu_print(EmulatorException& exception)
{
	UNUSED(exception);

	print();
}

void Emulator32bit::_emu_printr(byte reg_id, EmulatorException& exception)
{
	printf("REG: %d = %x\n", reg_id, read_reg(reg_id, exception));
}

void Emulator32bit::_emu_printm(word mem_addr, byte size, bool little_endian,
								EmulatorException& exception)
{
	word val = 0;
	if (little_endian) {
		for (byte i = 0; i < size; i++) {
			val <<= 8;
			val += system_bus.read_byte(mem_addr + i, exception.sys_bus_exception,
										exception.mem_read_exception);
		}
	} else {
		for (int i = size - 1; i >= 0; i--) {
			val <<= 8;
			val += system_bus.read_byte(mem_addr + i, exception.sys_bus_exception,
										exception.mem_read_exception);
		}
	}

	printf("MEM: %x = %.2x", mem_addr, val);
}

void Emulator32bit::_emu_printp(EmulatorException& exception)
{
	UNUSED(exception);

	printf("PSTATE: N=%lli,Z=%lli,C=%lli,V=%lli", test_bit(_pstate, N_FLAG), test_bit(_pstate, Z_FLAG),
		   test_bit(_pstate, C_FLAG), test_bit(_pstate, V_FLAG));
}

void Emulator32bit::_emu_assertr(byte reg_id, word min_value, word max_value,
								 EmulatorException& exception) {
	word val = read_reg(reg_id, exception);
	EXPECT_TRUE_SS(val >= min_value && val <= max_value, std::stringstream()
			<< "Expected register " << std::to_string(reg_id) << " to be between "
			<< std::to_string(min_value) << " and " << std::to_string(max_value) << ". Got "
			<< std::to_string(val));
}

void Emulator32bit::_emu_assertm(word mem_addr, byte size, bool little_endian, word min_value,
								 word max_value, EmulatorException& exception)
{
	word val = 0;
	if (little_endian) {
		for (byte i = 0; i < size; i++) {
			val <<= 8;
			val += system_bus.read_byte(mem_addr + i, exception.sys_bus_exception, exception.mem_read_exception);
		}
	} else {
		for (int i = size - 1; i >= 0; i--) {
			val <<= 8;
			val += system_bus.read_byte(mem_addr + i, exception.sys_bus_exception, exception.mem_read_exception);
		}
	}
	EXPECT_TRUE_SS(val >= min_value && val <= max_value, std::stringstream()
			<< "Expected value at memory address " << std::to_string(mem_addr) << " to be between "
			<< std::to_string(min_value) << " and " << std::to_string(max_value) << ". Got "
			<< std::to_string(val));
}

void Emulator32bit::_emu_assertp(byte p_state_id, bool expected_value, EmulatorException& exception)
{
	UNUSED(exception);

	bool val = test_bit(_pstate, p_state_id);
	EXPECT_TRUE_SS(val == expected_value, std::stringstream() << "Expected PSTATE " <<
			std::to_string(p_state_id) << " to be " << std::to_string(expected_value) << ". Got "
			<< std::to_string(val));
}

void Emulator32bit::_emu_log(word str, EmulatorException& exception)
{
	std::string msg;
	while (system_bus.read_byte(str, exception.sys_bus_exception, exception.mem_read_exception) != '\0') {
		msg += (char) system_bus.read_byte(str, exception.sys_bus_exception, exception.mem_read_exception);
		str++;
	}

	std::cout << msg << "\n";
}

void Emulator32bit::_emu_err(word err, EmulatorException& exception)
{
	std::string msg;
	while (system_bus.read_byte(err, exception.sys_bus_exception, exception.mem_read_exception) != '\0') {
		msg += (char) system_bus.read_byte(err, exception.sys_bus_exception, exception.mem_read_exception);
		err++;
	}

	std::cerr << msg << "\n";
}

/**
 * @brief					System Calls
 * 							https://chromium.googlesource.com/chromiumos/docs/+/master/constants/syscalls.md#arm64-64_bit
 * 							Would prefer to, in the future, implement these system calls directly in the kernel rather than abstracting it away
 * 							here. For now this would be fine until a higher level language is implemented for basm
 *
 * 							File management would be simulated through creating a large file to represent a hard drive (something along the lines of ~16 MiB)
 * ______________________________________________________________________________________________________________________________________________________________________________________________
 * | ID |		NAME	   |		arg x0		   |		arg x1		   |		arg x2		   |		arg x3			   |				arg x4				   |		arg x5			|
 * |____|__________________|_______________________|_______________________|_______________________|___________________________|_______________________________________|________________________|
 * |
 * |======================= Emulator Specific =======================
 * |1000: emu_print			-						-						-						-							-										-
 * |	- prints emulator state to console
 * |1001: emu_printr		byte reg_id				-						-						-							-										-
 * |	- prints a register to console
 * |1002: emu_printm		word mem_addr			byte size				bool little_endian		-							-										-
 * |	- prints a specific value in memory to console
 * |1003: emu_printp		-						-						-						-							-										-
 * |	- prints the pstate of the processor
 * |1010: emu_assertr		byte reg_id				word min_value			word max_value			-							-										-
 * |	- halts execution if reg val is not within bounds
 * |1011: emu_assertm		word mem_addr			byte size				bool little_endian		word min_value				word max_value							-
 * |	- halts execution if mem val is not within bounds
 * |1012: emu_assertp		byte p_state_id			bool expected_val		-						-							-										-
 * |	- halts execution if the specified p_state is not the expected val
 * |1020: emu_log			char* str				-						-						-							-										-
 * |	- prints message to console
 * |1021: emu_error			char* err				-						-						-							-										-
 * |	- prints merror to console and halts program
 * |
 * |======================= I/O Operations ==========================
 * |
 * | 0: io_setup			unsigned nr_reqs		aio_context_t *ctx
 * | 	- creates the context information for the I/O operation with space for #nr requests
 * | 1: io_destroy			aio_context_t ctx
 * | 	- invalidates the previously created context information
 * | 2: io_submit			aio_context_t			long					struct iocb * *
 * | 	- with the file descriptor (some unique number that identifies a specific file), begins an operation
 * | 3: io_cancel			aio_context_t ctx_id	struct iocb *iocb		struct io_event *result
 * | 	- cancels a specific I/O operation
 * | 4: io_getevents		aio_context_t ctx_id	long min_nr				long nr					struct io_event *events		struct __kernel_timespec *timeout
 * | 	- waits for when a specific I/O operation finishes or timesout
 * |
 * |======================= File Operations =========================
 * | 5: setxattr			const char *path		const char *name		const void *value		size_t size					int flags								-
 * |	-
 * | 6: lsetxattr			const char *path		const char *name		const void *value		size_t size					int flags								-
 * |	-
 * | 7: fsetxattr			int fd					const char *name		const void *value		size_t size					int flags								-
 * |	-
 * | 8: getxattr			const char *path		const char *name		void *value				size_t size					-										-
 * |	-
 * | 9: lgetxattr			const char *path		const char *name		void *value				size_t size					-										-
 * |	-
 * | 10: fgetxattr			int fd					const char *name		void *value				size_t size					-										-
 * |	-
 * | 11: listxattr			const char *path		char *list				size_t size				-							-										-
 * |	-
 * | 12: llistxattr			const char *path		char *list				size_t size				-							-										-
 * |	-
 * | 13: flistxattr			int fd					char *list				size_t size				-							-										-
 * |	-
 * | 14: removexattr		const char *path		const char *name		-						-							-										-
 * |	-
 * | 15: lremovexattr		const char *path		const char *name		-						-							-										-
 * |	-
 * | 16: fremovexattr		int fd					const char *name		-						-							-										-
 * |	-
 * L____________________________________________________________________________________________________________________________________________________________________________________________|
 * @param instr
 * @param exception
 */
void Emulator32bit::_swi(word instr, EmulatorException& exception)
{
	byte cond = bitfield_u32(instr, 22, 4);
	DEBUG_SS(std::stringstream() << "swi " << std::to_string(cond));

	if (!check_cond(_pstate, cond)) {
		return;
	}

	// software interrupts.. perfect to add functionality to this like console print,
	// file operations, ports, etc
	word id = read_reg(NR, exception);
	word arg0 = read_reg(0, exception);
	word arg1 = read_reg(1, exception);
	word arg2 = read_reg(2, exception);
	word arg3 = read_reg(3, exception);
	word arg4 = read_reg(4, exception);
	word arg5 = read_reg(5, exception);
	UNUSED(arg5); // temporary
	switch(id) {
		case 1000:
			_emu_print(exception);
			break;
		case 1001:
			_emu_printr(arg0, exception);
			break;
		case 1002:
			_emu_printm(arg0, arg1, arg2, exception);
			break;
		case 1003:
			_emu_printp(exception);
			break;

		case 1010:
			_emu_assertr(arg0, arg1, arg2, exception);
			break;
		case 1011:
			_emu_assertm(arg0, arg1, arg2, arg3, arg4, exception);
			break;
		case 1012:
			_emu_assertp(arg0, arg1, exception);
			break;
		default:
			ERROR_SS(std::stringstream() << "INVALID SYSCALL NUMBER: " << std::to_string(id));
	}
}