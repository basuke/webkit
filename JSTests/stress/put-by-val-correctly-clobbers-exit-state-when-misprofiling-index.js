let foo = $vm.createBuiltin(`(function (array, index) {
    index = @idWithProfile(index, "SpecObject");
    return array[index];
})`);
noInline(foo);

for (let i = 0; i < testLoopCount; ++i) {
    if (foo([1,2], 0) !== 1)
        throw new Error();
}
