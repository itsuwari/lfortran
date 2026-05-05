program c_backend_select_case_substring_cleanup_01
implicit none

character(12) :: line
integer :: first, last

line = "$coord rest"
first = 1
last = 6
select case (line(first:last))
case ("$coord")
    continue
case default
    error stop "expected coord"
end select

line = "$end trailing"
first = 1
last = 4
select case (line(first:last))
case ("$coord")
    error stop "expected end"
case ("$end")
    continue
case default
    error stop "expected end"
end select

end program
