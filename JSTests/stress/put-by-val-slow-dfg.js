function foo(a0) {
  a0[0] = undefined;
}

for (let i = 0; i < testLoopCount; i++) {
  foo(0);
  foo([]);
  foo('');
}
