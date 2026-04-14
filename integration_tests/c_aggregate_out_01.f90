program c_aggregate_out_01
    implicit none

    type :: payload_t
        integer :: value
    end type payload_t

    type(payload_t), allocatable :: tmp

    call init_payload(tmp)
    if (.not. allocated(tmp)) error stop
    if (tmp%value /= 42) error stop

contains

    subroutine init_payload(self)
        type(payload_t), intent(out) :: self
        self%value = 42
    end subroutine init_payload

end program c_aggregate_out_01
