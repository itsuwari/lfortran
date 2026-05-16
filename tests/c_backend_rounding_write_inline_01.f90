program c_backend_rounding_write_inline_01
implicit none

real(8) :: x

x = 2.6d0
write(*, '(i0,1x,i0)') nint(x), nint(-x)

end program
