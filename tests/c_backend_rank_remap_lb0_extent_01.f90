module c_backend_rank_remap_lb0_extent_01
    implicit none

    integer, parameter :: dp = kind(1.0d0)
    integer, parameter :: msao(0:1) = [1, 3]

    type :: shell_type
        integer :: ang = 1
    end type shell_type

contains

    subroutine fill_block(shell, out)
        type(shell_type), intent(in) :: shell
        real(dp), intent(out) :: out(msao(shell%ang), msao(shell%ang))

        out(:, :) = 2.0_dp
    end subroutine fill_block

    subroutine run_case()
        type(shell_type) :: shell
        real(dp) :: buffer(9)
        integer :: i

        shell%ang = 1
        buffer = 0.0_dp

        call fill_block(shell, buffer)

        do i = 1, 9
            if (buffer(i) /= 2.0_dp) error stop i
        end do
    end subroutine run_case

end module c_backend_rank_remap_lb0_extent_01

program test_c_backend_rank_remap_lb0_extent_01
    use c_backend_rank_remap_lb0_extent_01, only: run_case
    implicit none

    call run_case()
end program test_c_backend_rank_remap_lb0_extent_01
