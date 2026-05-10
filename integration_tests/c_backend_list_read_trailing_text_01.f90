program c_backend_list_read_trailing_text_01
implicit none
integer :: u, n
integer :: values(3)
integer :: more_values(3)
integer :: i

open(newunit=u, file="c_backend_list_read_trailing_text_01.tmp", status="replace", action="readwrite")
write(u, '(a)') "3                 number of values"
write(u, '(a)') "10 20 30          values"
write(u, '(a)') "40 50 60          implied-do values"
rewind(u)

read(u, *) n
read(u, *) values(1), values(2), values(3)
read(u, *) (more_values(i), i = 1, 3)

if (n /= 3) error stop
if (any(values /= [10, 20, 30])) error stop
if (any(more_values /= [40, 50, 60])) error stop

close(u, status="delete")
end program
