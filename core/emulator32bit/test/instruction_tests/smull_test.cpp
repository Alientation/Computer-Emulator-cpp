#include <emulator32bit_test/emulator32bit_test.h>

TEST(smull, register_smull_register) {
    Emulator32bit *cpu = new Emulator32bit(1, 0, {}, 0, 1);
    // smull x0, x1, x2, x3
    // x2: 2
    // x3: 4
    cpu->system_bus.write_word(0, Emulator32bit::asm_format_o2(Emulator32bit::_op_smull, false, 0, 1, 2, 3));
    cpu->set_pc(0);
    cpu->write_reg(2, 2);
    cpu->write_reg(3, 4);

    cpu->run(1);

    EXPECT_EQ(cpu->read_reg(0), 8) << "\'smull x0, x1, x2, x3\' : where x2=2, x3=4, should result in x0=8, x1=0";
    EXPECT_EQ(cpu->read_reg(1), 0) << "\'smull x0, x1, x2, x3\' : where x2=2, x3=4, should result in x0=8, x1=0";
    EXPECT_EQ(cpu->read_reg(2), 2) << "operation should not alter operand register \'x2\'";
    EXPECT_EQ(cpu->read_reg(3), 4) << "operation should not alter operand register \'x3\'";
    EXPECT_EQ(cpu->get_flag(N_FLAG), 0) << "operation should not cause N flag to be set";
    EXPECT_EQ(cpu->get_flag(Z_FLAG), 0) << "operation should not cause Z flag to be set";
    EXPECT_EQ(cpu->get_flag(C_FLAG), 0) << "operation should not cause C flag to be set";
    EXPECT_EQ(cpu->get_flag(V_FLAG), 0) << "operation should not cause V flag to be set";
    delete cpu;
}

TEST(smull, negative_flag) {
    Emulator32bit *cpu = new Emulator32bit(1, 0, {}, 0, 1);
    // smull x0, x1, x2, x3
    // x2: -2
    // x3: 4
    cpu->system_bus.write_word(0, Emulator32bit::asm_format_o2(Emulator32bit::_op_smull, true, 0, 1, 2, 3));
    cpu->set_pc(0);
    cpu->write_reg(2, -2);
    cpu->write_reg(3, 4);

    cpu->run(1);

    EXPECT_EQ(cpu->read_reg(0), -8) << "\'smull x0, x1, x2, x3\' : where x2=-2, x3=4, should result in x0=-8, x1=-1";
    EXPECT_EQ(cpu->read_reg(1), -1) << "\'smull x0, x1, x2, x3\' : where x2=-2, x3=4, should result in x0=-8, x1=-1";
    EXPECT_EQ(cpu->read_reg(2), -2) << "operation should not alter operand register \'x2\'";
    EXPECT_EQ(cpu->read_reg(3), 4) << "operation should not alter operand register \'x3\'";
    EXPECT_EQ(cpu->get_flag(N_FLAG), 1) << "N flag should be set";
    EXPECT_EQ(cpu->get_flag(Z_FLAG), 0) << "operation should not cause Z flag to be set";
    EXPECT_EQ(cpu->get_flag(C_FLAG), 0) << "operation should not cause C flag to be set";
    EXPECT_EQ(cpu->get_flag(V_FLAG), 0) << "operation should not cause V flag to be set";
    delete cpu;
}

TEST(smull, zero_flag) {
    Emulator32bit *cpu = new Emulator32bit(1, 0, {}, 0, 1);
    // smull x0, x1, x2, x3
    // x2: 0
    // x3: 4
    cpu->system_bus.write_word(0, Emulator32bit::asm_format_o2(Emulator32bit::_op_smull, true, 0, 1, 2, 3));
    cpu->set_pc(0);
    cpu->write_reg(2, 0);
    cpu->write_reg(3, 4);
    cpu->set_NZCV(0, 0, 1, 1);

    cpu->run(1);

    EXPECT_EQ(cpu->read_reg(0), 0) << "\'smull x0, x1, x2, x3\' : where x2=0, x3=4, should result in x0=0, x1=0";
    EXPECT_EQ(cpu->read_reg(1), 0) << "\'smull x0, x1, x2, x3\' : where x2=0, x3=4, should result in x0=0, x1=0";
    EXPECT_EQ(cpu->read_reg(2), 0) << "operation should not alter operand register \'x2\'";
    EXPECT_EQ(cpu->read_reg(3), 4) << "operation should not alter operand register \'x3\'";
    EXPECT_EQ(cpu->get_flag(N_FLAG), 0) << "operation should not cause N flag to be set";
    EXPECT_EQ(cpu->get_flag(Z_FLAG), 1) << "Z flag should be set";
    EXPECT_EQ(cpu->get_flag(C_FLAG), 1) << "operation should not alter C flag";
    EXPECT_EQ(cpu->get_flag(V_FLAG), 1) << "operation should not alter V flag";
    delete cpu;
}