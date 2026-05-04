program c_backend_c_char_assumed_size_01
use, intrinsic :: iso_c_binding, only: c_char, c_int, c_null_char, c_size_t
implicit none
character(kind=c_char, len=1) :: rhs(4)
character(len=:, kind=c_char), allocatable :: lhs
character(kind=c_char, len=1) :: bytes(4)
integer(c_int) :: dst_i(1)
integer(c_size_t) :: c_len

interface
    function c_strlen(charptr) result(n) bind(C, name="strlen")
        import :: c_char, c_size_t
        character(kind=c_char), intent(in) :: charptr(*)
        integer(c_size_t) :: n
    end function
end interface

rhs = [c_char_'a', c_char_'b', c_null_char, c_char_'x']
call c_f_character(rhs, lhs)

if (lhs /= c_char_'ab') error stop
c_len = c_strlen(as_c_char(c_char_'ab'))
if (c_len /= 2_c_size_t) error stop

bytes = [c_char_'a', c_char_'b', c_char_'c', c_char_'d']
dst_i = transfer(bytes, dst_i)
if (dst_i(1) == 0_c_int) error stop

contains

function load_method(charptr) result(n) bind(C)
    character(kind=c_char), intent(in) :: charptr(*)
    integer(c_int) :: n
    character(len=:, kind=c_char), allocatable :: method

    call c_f_character(charptr, method)
    if (method /= c_char_'ab') error stop
    n = len(method, kind=c_int)
end function

pure function as_c_char(str) result(res)
    character(len=*), intent(in) :: str
    character(kind=c_char) :: res(len(str)+1)

    res = transfer(str // c_null_char, res)
end function

subroutine read_chars(unit, var, stat)
    integer, intent(in) :: unit
    character(kind=c_char, len=1), intent(out) :: var(:)
    integer, intent(out) :: stat

    read(unit, iostat=stat) var
end subroutine

subroutine c_f_character(rhs, lhs)
    character(kind=c_char), intent(in) :: rhs(*)
    character(len=:, kind=c_char), allocatable, intent(out) :: lhs
    integer :: ii

    do ii = 1, huge(ii) - 1
        if (rhs(ii) == c_null_char) exit
    end do
    allocate(character(len=ii-1) :: lhs)
    lhs = transfer(rhs(1:ii-1), lhs)
end subroutine

end program
