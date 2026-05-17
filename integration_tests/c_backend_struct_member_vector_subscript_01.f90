program c_backend_struct_member_vector_subscript_01
implicit none

type :: item_t
    integer :: first
    integer :: last
end type

type(item_t), allocatable :: token(:)
integer :: line(3), shift(3)

allocate(token(4))
token(1)%first = 11
token(2)%first = 22
token(3)%first = 33
token(4)%first = 44

line = [4, 2, 3]
shift(:) = token(line)%first - 1

if (any(shift /= [43, 21, 32])) error stop

end program
