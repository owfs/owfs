;Runtime patch markers
%define _patch1_    01BCCAABh           ;Relative address of the EbMode function
%define _patch2_    02BCCAABh           ;Address of the previous WndProc
%define _patch3_    03BCCAABh           ;Relative address of SetWindowsLong
%define _patch4_    04BCCAABh           ;Address of the previous WndProc
%define _patch5_    05BCCAABh           ;Relative address of CallWindowProc
%define _patch6_    06BCCAABh           ;TableA entry count
%define _patch7_    07BCCAABh           ;TableA1
%define _patch8_    08BCCAABh           ;TableA2
%define _patch9_    09BCCAABh           ;TableB entry count
%define _patchA_    0ABCCAABh           ;TableB1
%define _patchB_    0BBCCAABh           ;TableB2

;Stack frame parameters and local variables. After "push edx" the ebp stack frame will look like this...
%define lParam      [ebp+24]            ;lParam parameter
%define wParam      [ebp+20]            ;wParam parameter
%define uMsg        [ebp+16]            ;Message number parameter
%define hWnd        [ebp+12]            ;Window handle parameter
%define Owner       [ebp+08]            ;Instance owner of message
       ;Information [ebp +4] return address to the caller
       ;Information [ebp +0] previous value of ebp pushed here, (implicitly restored with the leave statement)
              
%define GWL_WNDPROC     -4              ;SetWindowsLong WndProc offset
%define RESOLVE_MESSAGE 800h
%define SOCKET_MESSAGE  801h

[bits 32]   ;32bit code


;Entry point, setup stack frame
    pop     eax				;Retrieve address to the caller
    push    eax                         ;We make this push to reserve stack space for Owner
    push    eax                         ;Push back address to the caller
    push    ebp                         ;Preserve base pointer (ebp)
    mov     ebp, esp                    ;Create stack frame
    push    edi                         ;Preserve edi
    push    ebx                         ;Preserve ebx
    push    ecx                         ;Preserve ecx
    push    edx                         ;Preserve edx


;Initialize locals
    xor     eax, eax                    ;Clear eax
    jmp     _no_ide                     ;Patched with two nop's if running in the the IDE


;Check to see if the IDE is on a breakpoint or has stopped
    db      0E8h                        ;Far call
    dd      _patch1_                    ;Call EbMode, patched at runtime
    cmp     eax, 2                      ;If 2 
    je      _break                      ;  IDE is on a breakpoint, just call the original WndProc
    test    eax, eax                    ;If 0
    je      _unsub                      ;  IDE has stopped, unsubclass the window
    

_no_ide:
    mov     eax, uMsg                   ;Message number to search for
    cmp	    eax, dword RESOLVE_MESSAGE  ;message is RESOLVE_MESSAGE
    je	    _find_owner_RM              ;  find owner of the RESOLVE_MESSAGE
    cmp	    eax, dword SOCKET_MESSAGE   ;message is SOCKET_MESSAGE
    je      _find_owner_SM              ;  find owner of the SOCKET_MESSAGE
    call    _original                   ;message wasn't R_M neither S_M so we call original WndProc


_return:    ;Cleanup and exit
    pop     edx                         ;Restore edx
    pop     ecx                         ;Restore ecx
    pop     ebx                         ;Restore ebx
    pop     edi                         ;Restore edi
    leave                               ;Restore ebp and esp
    ret     20                          ;Return and adjust esp


_break:     ;The IDE is on a breakpoint, call the original WndProc and return
    call    _original
    jmp     _return


_unsub:     ;IDE has stopped, unsubclass the window
    push    _patch2_                    ;Address of the previous WndProc, patched at runtime
    push    dword GWL_WNDPROC           ;WndProc index
    push    dword hWnd                  ;Push the window handle
    db      0e8h                        ;Far call
    dd      _patch3_                    ;Relative address of SetWindowsLong, patched at runtime
    jmp     _return                     ;Return

    
_original:  ;Call original WndProc
    push    dword lParam                ;ByVal lParam
    push    dword wParam                ;ByVal wParam
    push    dword uMsg                  ;ByVal uMsg
    push    dword hWnd                  ;ByVal hWnd
    push    _patch4_                    ;Address of the previous WndProc, patched at runtime
    db      0e8h                        ;Far Call 
    dd      _patch5_                    ;Relative address of CallWindowProc, patched at runtime
    ret
    

_find_owner_RM:
    mov     ebx, _patch6_               ;TableA entry count
    mov     eax, wParam                 ;Async handle
    mov     edi, _patch7_               ;TableA1
    mov     ecx, ebx                    ;TableA entry count
    repne   scasd                       ;Scan the table
    jne     _return                     ;If the wParam number isn't found in the table just skip

    sub     ebx, ecx                    ;ebx = ebx - ecx
    dec     ebx                         ;ebx = ebx - 1
    mov     ebx, [ebx*4+_patch8_]       ;Now ebx is a pointer to async owner
    jmp     _call                       ;Prepare to make de callback

_find_owner_SM:
    mov     ebx, _patch9_               ;TableB entry count
    mov     eax, wParam                 ;Socket handle
    mov     edi, _patchA_               ;TableB1
    mov     ecx, ebx                    ;TableB entry count
    repne   scasd                       ;Scan the table
    jne     _return                     ;If the wParam number isn't found in the table just skip

    sub     ebx, ecx                    ;ebx = ebx - ecx
    dec     ebx                         ;ebx = ebx - 1
    mov     ebx, [ebx*4+_patchB_]       ;Now ebx is a pointer to socket owner
  
_call:      ;Callback to the owners
    mov     Owner, ebx                  ;Copy address of the owner object to the stack
    mov     ebx, [ebx]                  ;Get the address of the vTable into ebx
    mov     ebx, [ebx+1Ch]              ;Get the address of our WndProc into ebx
    mov     eax, ebx                    ;Copy the address of our WndProc into eax

    pop     edx                         ;Restore edx
    pop     ecx                         ;Restore ecx
    pop     ebx                         ;Restore ebx
    pop     edi                         ;Restore edi
    leave                               ;Restore ebp and esp
    
    jmp     eax                         ;Jump to our WndProc
    
