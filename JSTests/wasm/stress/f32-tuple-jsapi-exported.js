import { instantiate } from "../wabt-wrapper.js"
import * as assert from "../assert.js"

let wat = `
(module
    (func (export "test") (result f32 f32)
        (return (f32.const nan:0x100000) (f32.const nan:0x100000))
    )
)
`

async function test() {
    const instance = await instantiate(wat, {}, { simd: true })
    const { test } = instance.exports

    for (let i = 0; i < wasmTestLoopCount; ++i) {
        assert.eq(isNaN(test()[1]), true)
    }
}

await assert.asyncTest(test())
