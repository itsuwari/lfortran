program c_backend_fixed_char_intent_out_len_01
implicit none

character(len=1) :: bytes(8)
character(len=8) :: header
integer :: pos

bytes = ["A", "N", "U", "M", "P", "Y", "1", "0"]
pos = 0

call read_header(bytes, pos, header)

if (pos /= 8) error stop "wrong consumed length"
if (header(1:1) /= "A") error stop "wrong first byte"
if (header(2:6) /= "NUMPY") error stop "wrong magic string"
if (header(7:8) /= "10") error stop "wrong version bytes"
if (to_fixed_symbol() /= "OK") error stop "wrong function result symbol"

contains

    subroutine read_header(buffer, pos, out)
    character(len=1), intent(in) :: buffer(:)
    integer, intent(inout) :: pos
    character(len=*), intent(out) :: out
    integer :: i

    if (pos + len(out) > size(buffer)) error stop "buffer exhausted"
    do i = 1, len(out)
        out(i:i) = buffer(pos + i)
    end do
    pos = pos + len(out)
    end subroutine read_header

    function to_fixed_symbol() result(symbol)
    character(len=2) :: symbol

    call fill_fixed_symbol(symbol)
    end function to_fixed_symbol

    subroutine fill_fixed_symbol(out)
    character(len=2), intent(out) :: out

    out = "OK"
    end subroutine fill_fixed_symbol

end program c_backend_fixed_char_intent_out_len_01
