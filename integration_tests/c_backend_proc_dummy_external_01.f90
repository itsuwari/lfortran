program c_backend_proc_dummy_external_01
implicit none

logical :: ok

ok = .true.
if (.not. ok) error stop

end program

logical function apply_select(wr, wi, select)
implicit none
logical, external :: select
double precision, intent(in) :: wr, wi

apply_select = select(wr, wi)
end function

logical function is_selected(wr, wi)
implicit none
double precision, intent(in) :: wr, wi

is_selected = wr > wi
end function
