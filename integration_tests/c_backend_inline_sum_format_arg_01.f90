program c_backend_inline_sum_format_arg_01
implicit none

real(8) :: a(2, 2), b(2, 2)
character(len=64) :: line

a = reshape([1.0d0, 2.0d0, 3.0d0, 4.0d0], [2, 2])
b = reshape([5.0d0, 6.0d0, 7.0d0, 8.0d0], [2, 2])

write(line, '(f10.3)') sum(a * b)
if (index(line, "70.000") == 0) error stop

end program c_backend_inline_sum_format_arg_01
