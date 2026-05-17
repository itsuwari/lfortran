program c_backend_fixed_char_array_source_fill_01
implicit none

type :: atom_box
    character(len=4), allocatable :: label(:)
end type

character(len=4), allocatable :: sym(:), copy(:)
type(atom_box) :: lhs, rhs
integer :: i

allocate(sym(5), source='H   ')

do i = 1, size(sym)
    if (sym(i) /= 'H   ') error stop
end do

copy = sym

do i = 1, size(copy)
    if (copy(i) /= 'H   ') error stop
end do

sym = 'Fe'

do i = 1, size(sym)
    if (sym(i) /= 'Fe  ') error stop
end do

do i = 1, size(copy)
    if (copy(i) /= 'H   ') error stop
end do

deallocate(copy)

copy = sym(5:1:-2)

if (size(copy) /= 3) error stop
if (copy(1) /= 'Fe  ') error stop
if (copy(2) /= 'Fe  ') error stop
if (copy(3) /= 'Fe  ') error stop

deallocate(copy)

sym = [character(len=4) ::]

if (size(sym) /= 0) error stop
if (.not. allocated(sym)) error stop

deallocate(sym)
allocate(sym(3), source='O   ')

do i = 1, size(sym)
    if (sym(i) /= 'O   ') error stop
end do

deallocate(sym)

allocate(lhs%label(2), source='Cu  ')
rhs = lhs

lhs%label = 'Zn'

if (rhs%label(1) /= 'Cu  ') error stop
if (rhs%label(2) /= 'Cu  ') error stop
if (lhs%label(1) /= 'Zn  ') error stop
if (lhs%label(2) /= 'Zn  ') error stop

deallocate(lhs%label)
deallocate(rhs%label)

allocate(sym(0), source='Q   ')

if (size(sym) /= 0) error stop
if (.not. allocated(sym)) error stop

deallocate(sym)

end program
