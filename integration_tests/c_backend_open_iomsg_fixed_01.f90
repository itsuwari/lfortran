program c_backend_open_iomsg_fixed_01
implicit none

integer :: ios, unit
character(len=512) :: msg

ios = 0
msg = ""
open(newunit=unit, file="c_backend_open_iomsg_fixed_01_missing.tmp", &
    status="old", iostat=ios, iomsg=msg)

if (ios == 0) then
    close(unit, status="delete")
    error stop "missing file unexpectedly opened"
end if
if (len_trim(msg) == 0) error stop "empty iomsg"

end program c_backend_open_iomsg_fixed_01
