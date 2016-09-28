	.section .tl420data, #alloc

	.globl	tl420_input_code
tl420_input_code:
	.incbin	"tl420_code"
	.globl	tl420_input_code_end
tl420_input_code_end:

	.globl	tl420_input_data
tl420_input_data:
	.incbin	"tl420_data"
	.globl	tl420_input_data_end
tl420_input_data_end:
