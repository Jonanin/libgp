
#include "cgp.h"
#include "mem.h"

#include <string.h>

// A statement is created as a random operation with random arguments.
// Each argument can be a register, constant, or input.
GpStatement gp_statement_random(GpWorld * world)
{
	uint j;

	GpStatement stmt;
	stmt.output = urand(0, world->conf.num_registers);
	stmt.op = &world->ops[urand(0, world->num_ops)];

	uint randopt = world->conf.num_inputs == 0 ? 2 : 3;

	for (j = 0; j < stmt.op->num_args; j++) {
		switch (urand(0, randopt)) {
		case 0:
			stmt.args[j].type = GP_ARG_REGISTER;
			stmt.args[j].data.reg = urand(0, world->conf.num_registers);
			break;
		case 1:
			stmt.args[j].type = GP_ARG_CONSTANT;
			stmt.args[j].data.num = world->conf.constant_func();
			break;
		case 2:
			stmt.args[j].type = GP_ARG_INPUT;
			stmt.args[j].data.reg = urand(0, world->conf.num_inputs);
			break;
		}
	}
	return stmt;
}

// Create a program with a random length N and a sequence of N randomly
// initialized statements
void gp_program_init(GpWorld * world, GpProgram * program)
{
	uint i;
	program->evaluated = 0;
	program->num_stmts = urand(world->conf.min_program_length,
							   world->conf.max_program_length + 1);
	program->stmts = new_array(GpStatement, program->num_stmts);
	for (i = 0; i < program->num_stmts; i++)
		program->stmts[i] = gp_statement_random(world);
}

GpProgram * gp_program_new(GpWorld * world)
{
	GpProgram * program = new(GpProgram);
	gp_program_init(world, program);
	return program;
}

// Copy all of one program's satements and data to another
// **Does NOT** free `dst`'s stmts.
void gp_program_copy(GpProgram * src, GpProgram * dst)
{
	dst->num_stmts = src->num_stmts;
	dst->stmts = new_array(GpStatement, dst->num_stmts);
	memcpy(dst->stmts, src->stmts, dst->num_stmts * sizeof(GpStatement));
}

void gp_program_delete(GpProgram * program)
{
	delete(program->stmts);
	delete(program);
}

// Test if two programs are _relatively_ equal
int gp_program_equal(GpProgram * a, GpProgram * b)
{
	uint i;

	if (a->num_stmts != b->num_stmts)
		return 0;

	for (i = 0; i < a->num_stmts; i++) {
		if (a->stmts[i].op != b->stmts[i].op)
			return 0;

		if (a->stmts[i].output != b->stmts[i].output)
			return 0;
	}

	return 1;
}

// Print out a program's instructions, one per line
void gp_program_print(GpProgram * program, FILE * f)
{
	uint i, j;
	for (i = 0; i < program->num_stmts; i++)
	{
		GpStatement * stmt = &program->stmts[i];
		fprintf(f, "r%u = %s ", stmt->output, stmt->op->name);
		for (j = 0; j < stmt->op->num_args; j++)
		{
			switch (stmt->args[j].type)
			{
			case GP_ARG_REGISTER:
				fprintf(f, "r%u", stmt->args[j].data.reg);
				break;
			case GP_ARG_CONSTANT:
				fprintf(f, "%f", stmt->args[j].data.num);
				break;
			case GP_ARG_INPUT:
				fprintf(f, "i%u", stmt->args[j].data.reg);
				break;
			default:
				printf("<unknown>");
			}
			if (j != stmt->op->num_args - 1)
				fprintf(f, ", ");
		}
		fprintf(f, "\n");
	}
}

// `gp_program_run` will execute the supplied `program` given inputs
// and return the final run state
GpState gp_program_run(GpWorld * world, GpProgram * program, gp_num_t * inputs)
{
	GpState state;
	state.ip = 0;
	memset(state.registers, 0, world->conf.num_registers * sizeof(gp_num_t));
	state.inputs = inputs;
	while (state.ip < program->num_stmts)
	{
		GpStatement * stmt = program->stmts + state.ip;
		(stmt->op->func)(&state, stmt->args, state.registers + stmt->output);
		state.ip++;
	}
	return state;
}
