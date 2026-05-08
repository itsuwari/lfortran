program c_backend_string_item_concat_alloc_01
implicit none

character(len=:), allocatable :: out

out = compact_copy("8 0")

if (out /= "80") error stop 1

contains

    function compact_copy(str) result(lcstr)
    character(len=*), intent(in) :: str
    character(len=:), allocatable :: lcstr
    integer :: i

    lcstr = ""
    do i = 1, len(str)
        if (str(i:i) == " ") cycle
        lcstr = lcstr // str(i:i)
    end do
    lcstr = trim(lcstr)
    end function compact_copy

end program c_backend_string_item_concat_alloc_01
