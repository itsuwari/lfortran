subroutine c_backend_common_block_loop_01_sub()
implicit none
integer :: i
common /c_backend_common_block_loop_01_common/ i

do i = 1, 3
end do

if (i /= 4) error stop
end subroutine

program c_backend_common_block_loop_01
implicit none
call c_backend_common_block_loop_01_sub()
end program
