program fixed_size_member_section_01
  implicit none

  type :: t
    integer :: n = 0
    real(8) :: a(4) = 0d0
  end type t

  type(t) :: x

  x%n = 2
  x%a = [3d0, 1d0, 9d0, 8d0]

  if (minval(x%a(:x%n)) /= 1d0) error stop "minval"
end program fixed_size_member_section_01
