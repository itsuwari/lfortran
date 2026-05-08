program c_backend_alloc_char_array_init_01
implicit none
integer :: i
character(len=3), allocatable :: sym(:)
character(len=:), allocatable :: symbol

allocate(sym(4))

do i = 1, size(sym)
    select case (i)
    case (1)
        symbol = "N  "
    case (2)
        symbol = "C  "
    case (3)
        symbol = "O  "
    case default
        symbol = "H  "
    end select
    sym(i) = symbol
end do

if (sym(1) /= "N  ") error stop "bad first symbol"
if (sym(2) /= "C  ") error stop "bad second symbol"
if (sym(3) /= "O  ") error stop "bad third symbol"
if (sym(4) /= "H  ") error stop "bad fourth symbol"

end program
