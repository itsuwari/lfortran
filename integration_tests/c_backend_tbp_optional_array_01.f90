module c_backend_tbp_optional_array_01_m
implicit none

type :: model_type
contains
    procedure :: local_charge
end type

contains

subroutine local_charge(self, qloc, dqlocdr, dqlocdL)
    class(model_type), intent(in) :: self
    real(8), intent(out) :: qloc(:)
    real(8), intent(out), optional :: dqlocdr(3, size(qloc), size(qloc))
    real(8), intent(out), optional :: dqlocdL(3, 3, size(qloc))

    qloc = 1.0d0
    if (present(dqlocdr) .or. present(dqlocdL)) error stop
end subroutine

subroutine call_local_charge(model, qloc)
    class(model_type), intent(in) :: model
    real(8), intent(out) :: qloc(:)
    real(8), allocatable :: dqlocdr(:, :, :)
    real(8), allocatable :: dqlocdL(:, :, :)

    call model%local_charge(qloc, dqlocdr, dqlocdL)
end subroutine

end module

program c_backend_tbp_optional_array_01
use c_backend_tbp_optional_array_01_m, only: model_type, call_local_charge
implicit none

type(model_type) :: model
real(8) :: qloc(4)

call call_local_charge(model, qloc)
if (any(abs(qloc - 1.0d0) > 1.0d-12)) error stop

end program
