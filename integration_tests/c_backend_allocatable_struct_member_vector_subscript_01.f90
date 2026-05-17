program c_backend_allocatable_struct_member_vector_subscript_01
implicit none

type :: record_t
    integer :: nsh
end type

type(record_t), allocatable :: record(:)
integer, allocatable :: nsh_id(:)
integer :: irc(4)

allocate(record(5))
record(1)%nsh = 2
record(2)%nsh = 5
record(3)%nsh = 3
record(4)%nsh = 8
record(5)%nsh = 1

irc = [4, 2, 5, 3]
nsh_id = record(irc)%nsh

if (size(nsh_id) /= 4) error stop
if (any(nsh_id /= [8, 5, 1, 3])) error stop
if (maxval(record(irc)%nsh) /= 8) error stop

end program
