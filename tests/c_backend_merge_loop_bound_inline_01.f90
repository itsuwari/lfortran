program c_backend_merge_loop_bound_inline_01
implicit none

integer :: ix, jx, count, total

ix = 1
count = 0
total = 0
do jx = 1, merge(-1, 1, ix > 0), merge(-2, 2, ix > 0)
    count = count + 1
    total = total + jx
end do
if (count /= 2 .or. total /= 0) error stop

ix = -1
count = 0
total = 0
do jx = 1, merge(-1, 3, ix > 0), merge(-2, 1, ix > 0)
    count = count + 1
    total = total + jx
end do
if (count /= 3 .or. total /= 6) error stop

end program
