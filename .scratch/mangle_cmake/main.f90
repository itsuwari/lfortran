program main
  use mod_a, only: foo
  real(8) :: a(3), b(3)
  a = [1d0, 2d0, 3d0]
  call foo(a, b)
  if (any(abs(b - [2d0, 4d0, 6d0]) > 1d-12)) error stop 1
  print *, b
end program
