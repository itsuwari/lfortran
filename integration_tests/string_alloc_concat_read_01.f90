program main
implicit none

character(len=:), allocatable :: line, lattice_string
integer :: i, stat
real(8) :: vals(9)

lattice_string = ''
do i = 1, 3
    select case (i)
    case (1)
        line = '1.0 2.0 3.0'
    case (2)
        line = '2.0 3.0 4.0'
    case default
        line = '3.0 4.0 5.0'
    end select
    lattice_string = lattice_string // ' ' // line
end do

read(lattice_string, *, iostat=stat) vals
if (stat /= 0) error stop
if (abs(vals(1) - 1.0d0) > 1.0d-12) error stop
if (abs(vals(9) - 5.0d0) > 1.0d-12) error stop
end program
