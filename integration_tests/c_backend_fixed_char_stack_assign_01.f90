module c_backend_fixed_char_stack_assign_01_m
implicit none

contains

    integer function choose(flag, input)
    logical, intent(in) :: flag
    character(len=1), intent(in), optional :: input
    character(len=1) :: local

    if (flag .and. present(input)) then
        local = input
    else
        local = 'n'
    end if

    select case (local)
    case ('t')
        choose = 1
    case ('n')
        choose = 2
    case default
        choose = 3
    end select
    end function choose

end module c_backend_fixed_char_stack_assign_01_m

program c_backend_fixed_char_stack_assign_01
use c_backend_fixed_char_stack_assign_01_m, only: choose
implicit none

if (choose(.false.) /= 2) error stop
if (choose(.true., 't') /= 1) error stop
if (choose(.true., 'x') /= 3) error stop

end program c_backend_fixed_char_stack_assign_01
