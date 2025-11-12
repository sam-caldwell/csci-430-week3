; === Source: demo/example.fakelang ===
; 1 | // Fakelang demo: classes, inheritance, and virtual dispatch
; 2 | class Animal {
; 3 |   // Virtual method in base class
; 4 |   virtual speak(): String {
; 5 |     return "Animal";
; 6 |   }
; 7 | }
; 8 | 
; 9 | class Dog extends Animal {
; 10 |   // Override of virtual method
; 11 |   override speak(): String {
; 12 |     return "Woof";
; 13 |   }
; 14 | }
; 15 | 
; 16 | function main(): Int {
; 17 |   var a: Animal = new Animal();
; 18 |   var d: Animal = new Dog();
; 19 |   print(a.speak());
; 20 |   print(d.speak());
; 21 |   return 0;
; 22 | }
; 23 | 
; === LLVM Module IR ===
; ModuleID = 'demo/example.fakelang'
source_filename = "fakelang-module"

%vtable.Animal = type { ptr }
%vtable.Dog = type { ptr }
%class.Animal = type { ptr }
%class.Dog = type { ptr }

@0 = private unnamed_addr constant [7 x i8] c"Animal\00", align 1
@1 = private unnamed_addr constant [5 x i8] c"Woof\00", align 1
@vtable.Animal = private constant %vtable.Animal { ptr @Animal.speak }
@vtable.Dog = private constant %vtable.Dog { ptr @Dog.speak }

;
; === Function: Animal.speak ===
define ptr @Animal.speak(ptr %0) {
entry:
 ; src: demo/example.fakelang:5:5-5:21 | return | return "Animal";  ret ptr @0, !fakelang.src !0
}

;
; === Function: Dog.speak ===
define ptr @Dog.speak(ptr %0) {
entry:
 ; src: demo/example.fakelang:12:5-12:19 | return | return "Woof";  ret ptr @1, !fakelang.src !1
}

;
; === Function: puts ===
declare i32 @puts(ptr)

;
; === Function: main ===
define i32 @main() {
entry:
 ; src: demo/example.fakelang:17:3-17:32 | alloca var | var a: Animal = new Animal();  %a.addr = alloca ptr, align 8, !fakelang.src !2
 ; src: demo/example.fakelang:17:19-17:31 | alloca object | new Animal();  %Animal.obj = alloca %class.Animal, align 8, !fakelang.src !3
 ; src: demo/example.fakelang:17:19-17:31 | vptr addr | new Animal();  %Animal.vptr.addr = getelementptr inbounds %class.Animal, ptr %Animal.obj, i32 0, i32 0, !fakelang.src !4
 ; src: demo/example.fakelang:17:19-17:31 | store vptr | new Animal();  store ptr @vtable.Animal, ptr %Animal.vptr.addr, align 8, !fakelang.src !5
 ; src: demo/example.fakelang:17:3-17:32 | store var | var a: Animal = new Animal();  store ptr %Animal.obj, ptr %a.addr, align 8, !fakelang.src !6
 ; src: demo/example.fakelang:18:3-18:29 | alloca var | var d: Animal = new Dog();  %d.addr = alloca ptr, align 8, !fakelang.src !7
 ; src: demo/example.fakelang:18:19-18:28 | alloca object | new Dog();  %Dog.obj = alloca %class.Dog, align 8, !fakelang.src !8
 ; src: demo/example.fakelang:18:19-18:28 | vptr addr | new Dog();  %Dog.vptr.addr = getelementptr inbounds %class.Dog, ptr %Dog.obj, i32 0, i32 0, !fakelang.src !9
 ; src: demo/example.fakelang:18:19-18:28 | store vptr | new Dog();  store ptr @vtable.Dog, ptr %Dog.vptr.addr, align 8, !fakelang.src !10
 ; src: demo/example.fakelang:18:3-18:29 | store var | var d: Animal = new Dog();  store ptr %Dog.obj, ptr %d.addr, align 8, !fakelang.src !11
 ; src: demo/example.fakelang:19:9-19:18 | load this | a.speak());  %a.val = load ptr, ptr %a.addr, align 8, !fakelang.src !12
 ; src: demo/example.fakelang:19:9-19:18 | vptr addr | a.speak());  %Animal.vptr.addr1 = getelementptr inbounds %class.Animal, ptr %a.val, i32 0, i32 0, !fakelang.src !13
 ; src: demo/example.fakelang:19:9-19:18 | load vptr | a.speak());  %Animal.vptr = load ptr, ptr %Animal.vptr.addr1, align 8, !fakelang.src !14
 ; src: demo/example.fakelang:19:9-19:18 | slot addr | a.speak());  %speak.slot.addr = getelementptr inbounds %vtable.Animal, ptr %Animal.vptr, i32 0, i32 0, !fakelang.src !15
 ; src: demo/example.fakelang:19:9-19:18 | bitcast fn | a.speak());  %speak.slot = load ptr, ptr %speak.slot.addr, align 8, !fakelang.src !16
 ; src: demo/example.fakelang:19:9-19:18 | vcall | a.speak());  %speak.call = call ptr %speak.slot(ptr %a.val), !fakelang.src !17
 ; src: demo/example.fakelang:19:3-19:20 | print | print(a.speak());  %0 = call i32 @puts(ptr %speak.call), !fakelang.src !18
 ; src: demo/example.fakelang:20:9-20:18 | load this | d.speak());  %d.val = load ptr, ptr %d.addr, align 8, !fakelang.src !19
 ; src: demo/example.fakelang:20:9-20:18 | vptr addr | d.speak());  %Animal.vptr.addr2 = getelementptr inbounds %class.Animal, ptr %d.val, i32 0, i32 0, !fakelang.src !20
 ; src: demo/example.fakelang:20:9-20:18 | load vptr | d.speak());  %Animal.vptr3 = load ptr, ptr %Animal.vptr.addr2, align 8, !fakelang.src !21
 ; src: demo/example.fakelang:20:9-20:18 | slot addr | d.speak());  %speak.slot.addr4 = getelementptr inbounds %vtable.Animal, ptr %Animal.vptr3, i32 0, i32 0, !fakelang.src !22
 ; src: demo/example.fakelang:20:9-20:18 | bitcast fn | d.speak());  %speak.slot5 = load ptr, ptr %speak.slot.addr4, align 8, !fakelang.src !23
 ; src: demo/example.fakelang:20:9-20:18 | vcall | d.speak());  %speak.call6 = call ptr %speak.slot5(ptr %d.val), !fakelang.src !24
 ; src: demo/example.fakelang:20:3-20:20 | print | print(d.speak());  %1 = call i32 @puts(ptr %speak.call6), !fakelang.src !25
 ; src: demo/example.fakelang:21:3-21:12 | return | return 0;  ret i32 0, !fakelang.src !26
}

!0 = !{!"demo/example.fakelang:5:5-5:21 | return | return \22Animal\22;"}
!1 = !{!"demo/example.fakelang:12:5-12:19 | return | return \22Woof\22;"}
!2 = !{!"demo/example.fakelang:17:3-17:32 | alloca var | var a: Animal = new Animal();"}
!3 = !{!"demo/example.fakelang:17:19-17:31 | alloca object | new Animal();"}
!4 = !{!"demo/example.fakelang:17:19-17:31 | vptr addr | new Animal();"}
!5 = !{!"demo/example.fakelang:17:19-17:31 | store vptr | new Animal();"}
!6 = !{!"demo/example.fakelang:17:3-17:32 | store var | var a: Animal = new Animal();"}
!7 = !{!"demo/example.fakelang:18:3-18:29 | alloca var | var d: Animal = new Dog();"}
!8 = !{!"demo/example.fakelang:18:19-18:28 | alloca object | new Dog();"}
!9 = !{!"demo/example.fakelang:18:19-18:28 | vptr addr | new Dog();"}
!10 = !{!"demo/example.fakelang:18:19-18:28 | store vptr | new Dog();"}
!11 = !{!"demo/example.fakelang:18:3-18:29 | store var | var d: Animal = new Dog();"}
!12 = !{!"demo/example.fakelang:19:9-19:18 | load this | a.speak());"}
!13 = !{!"demo/example.fakelang:19:9-19:18 | vptr addr | a.speak());"}
!14 = !{!"demo/example.fakelang:19:9-19:18 | load vptr | a.speak());"}
!15 = !{!"demo/example.fakelang:19:9-19:18 | slot addr | a.speak());"}
!16 = !{!"demo/example.fakelang:19:9-19:18 | bitcast fn | a.speak());"}
!17 = !{!"demo/example.fakelang:19:9-19:18 | vcall | a.speak());"}
!18 = !{!"demo/example.fakelang:19:3-19:20 | print | print(a.speak());"}
!19 = !{!"demo/example.fakelang:20:9-20:18 | load this | d.speak());"}
!20 = !{!"demo/example.fakelang:20:9-20:18 | vptr addr | d.speak());"}
!21 = !{!"demo/example.fakelang:20:9-20:18 | load vptr | d.speak());"}
!22 = !{!"demo/example.fakelang:20:9-20:18 | slot addr | d.speak());"}
!23 = !{!"demo/example.fakelang:20:9-20:18 | bitcast fn | d.speak());"}
!24 = !{!"demo/example.fakelang:20:9-20:18 | vcall | d.speak());"}
!25 = !{!"demo/example.fakelang:20:3-20:20 | print | print(d.speak());"}
!26 = !{!"demo/example.fakelang:21:3-21:12 | return | return 0;"}
