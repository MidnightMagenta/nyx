Tasks hold a pointer to a virtual memory management struct and a pointer to the active mm.
If the mm_struct pointer is NULL, the task borrows whatever is `active_mm` in the previous task.
The task creator is responsible for assigning the task an address space (creating mm_struct)
