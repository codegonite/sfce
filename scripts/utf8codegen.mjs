import fs from "node:fs"
import path from "node:path"
import https from "node:https"
import url from 'url';

const __filename = url.fileURLToPath(import.meta.url);

const UNICODE_INDEX_CODE = 0
const UNICODE_INDEX_NAME = 1
const UNICODE_INDEX_CATEGORY = 2
const UNICODE_INDEX_COMBINING_CLASS = 3
const UNICODE_INDEX_BIDI_CLASS = 4
const UNICODE_INDEX_DECOMPOSITION = 5
const UNICODE_INDEX_DECIMAL_DIGIT = 6
const UNICODE_INDEX_DIGIT = 7
const UNICODE_INDEX_NUMERIC = 8
const UNICODE_INDEX_BIDI_MIRRORED = 9
const UNICODE_INDEX_UNICODE1_0_NAME = 10
const UNICODE_INDEX_ISO_COMMENT = 11
const UNICODE_INDEX_UPPERCASE_MAPPING = 12
const UNICODE_INDEX_LOWERCASE_MAPPING = 13
const UNICODE_INDEX_TITLE_CASE_MAPPING = 14

const characterClassWidthMap = new Map([
    [ "W",  2 ],
    [ "F",  2 ],
    [ "Na", 1 ],
    [ "H",  1 ],
    [ "A", -1 ],
])

class WritableByteStream {
    #size = 0
    #buffer = new Uint8Array(0x1000)

    static roundNearestKiloByte(value) {
        value = value | 0
        return (value + 0x0400 - 1) & -0x0400
    }

    write(chunk, callback) {
        const requestedSize = this.#size + chunk.length
        if (requestedSize >= this.#buffer.length) {
            const temp = this.#buffer
            const capacity = WritableByteStream.roundNearestKiloByte(requestedSize)
            this.#buffer = new Uint8Array(capacity)
            this.#buffer.set(temp)
        }

        this.#buffer.set(chunk, this.#size)
        this.#size += chunk.length

        if (callback) {
            callback()
        }
    }

    bytes() { return this.#buffer.subarray(0, this.#size) }
    size() { return this.#size }
}

function getContentsFromURL(url) {
    return new Promise((resolve, reject) => {
        const request = https.get(url, (response) => {
            const decoder = new TextDecoder('utf-8')
            const contents = new WritableByteStream()

            response.on("data", (chunk) => {
                contents.write(chunk)
            })

            response.on("end", () => {
                const result = contents.bytes()
                resolve(decoder.decode(result))
            })
        })

        request.on("error", error => reject(error))
    })
}

function hexCharacterToDecimal(character) {
    const characterCode = character.charCodeAt(0)

    if (characterCode >= 0x30 && characterCode <= 0x39) {
        return characterCode - 0x30
    }
    else if (characterCode >= 0x41 && characterCode <= 0x46) {
        return characterCode - 0x37
    }
    else if (characterCode >= 0x61 && characterCode <= 0x66) {
        return characterCode - 0x57
    }

    return null
}

function readHexRanges(content) {
    const result = []

    for (let idx = 0; idx < content.length;) {
        let digit, range0 = null, range1 = null, description0 = "", description1 = ""

        if (hexCharacterToDecimal(content[idx]) != null) {
            for (range0 = 0; digit = hexCharacterToDecimal(content[idx]), digit != null; ++idx) {
                range0 = (range0 << 4) + digit
            }
        }

        if (content[idx] == "." && content[idx + 1] == ".") {
            idx += 2

            if (hexCharacterToDecimal(content[idx]) != null) {
                for (range1 = 0; digit = hexCharacterToDecimal(content[idx]), digit != null; ++idx) {
                    range1 = (range1 << 4) + digit
                }
            }
        }

        while (content[idx] != "\r" && content[idx] != "\n") {
            if (content[idx] == ";") {
                ++idx

                while (content[idx] != "\r" && content[idx] != "\n" && content[idx] != "#") {
                    description0 += content[idx++]
                }

                description0 = description0.trim()
                continue
            }

            if (content[idx] == "#") {
                ++idx

                while (content[idx] != "\r" && content[idx] != "\n") {
                    description1 += content[idx++]
                }

                description1 = description1.trim()
                continue
            }

            ++idx
        }

        while (content[idx] == "\r" || content[idx] == "\n") ++idx

        if (range0 != null) {
            result.push({ range0, range1, description0, description1 })
        }
    }

    return result
}

function createSetFromRanges(ranges) {
    const result = new Set()

    for (const { range0, range1 } of ranges) {
        if (range1 != null) {
            for (let idx = range0; idx <= range1; ++idx) {
                result.add(idx)
            }
        }
        else {
            result.add(range0)
        }
    }

    return result
}

function toLines(string) {
    const result = string.split(/\r\n|[\r\n]/g)

    while (result[result.length - 1].length == 0) {
        result.splice(result.length - 1, 1)
    }

    return result
}

function parseHexadecimal(string) {
    if (string.length == 0 || hexCharacterToDecimal(string[0]) == null) {
        return null
    }

    let result = 0, idx = 0, digit = 0
    while (idx < string.length) {
        if (digit = hexCharacterToDecimal(string[idx++]), digit == null) {
            break
        }

        result = (result << 4) + digit
    }

    return result
}

function readUnicodeData(unicodeDataContents, uppercase, lowercase) {
    const unicodeData = []

    for (const line of toLines(unicodeDataContents)) {
        const unicodeColumns = line.split(";")
        const decomposition = unicodeColumns[UNICODE_INDEX_DECOMPOSITION]

        const decompositionParts = decomposition.split(" ");
        const hasDecompositionType = decomposition[0] == "<"
        const hasDecompositionMapping = decompositionParts.length != 0 && (
            decompositionParts.length != 1 || decompositionParts[0].length != 0
        )

        const code = parseHexadecimal(unicodeColumns[UNICODE_INDEX_CODE])
        const name = unicodeColumns[UNICODE_INDEX_NAME]
        const generalCategory = unicodeColumns[UNICODE_INDEX_CATEGORY]
        const canonicalCombiningClass = parseHexadecimal(unicodeColumns[UNICODE_INDEX_COMBINING_CLASS])
        const bidiClass = unicodeColumns[UNICODE_INDEX_BIDI_CLASS]
        const decompositionType = hasDecompositionType ? decompositionParts.shift() : null;
        const decompositionMappingString = hasDecompositionMapping ? decompositionParts : null
        const decimalDigit = unicodeColumns[UNICODE_INDEX_DECIMAL_DIGIT]
        const digit = unicodeColumns[UNICODE_INDEX_DIGIT]
        const numeric = unicodeColumns[UNICODE_INDEX_NUMERIC]
        const bidiMirrored = unicodeColumns[UNICODE_INDEX_BIDI_MIRRORED]
        const unicode1_0Name = unicodeColumns[UNICODE_INDEX_UNICODE1_0_NAME]
        const isoComment = unicodeColumns[UNICODE_INDEX_ISO_COMMENT]
        let simpleUppercaseMapping = parseHexadecimal(unicodeColumns[UNICODE_INDEX_UPPERCASE_MAPPING])
        let simpleLowercaseMapping = parseHexadecimal(unicodeColumns[UNICODE_INDEX_LOWERCASE_MAPPING])
        let simpleTitleCaseMapping = parseHexadecimal(unicodeColumns[UNICODE_INDEX_TITLE_CASE_MAPPING])
        const decompositionMapping = decompositionMappingString?.map(e => parseHexadecimal(e)) ?? null

        const hasUppercaseMapping = simpleUppercaseMapping !== null || simpleUppercaseMapping !== undefined
        const hasLowercaseMapping = simpleLowercaseMapping !== null || simpleLowercaseMapping !== undefined

        if (!hasLowercaseMapping && !hasUppercaseMapping) {
            if (lowercase.has(code)) {
                simpleUppercaseMapping = code
            }

            if (uppercase.has(code)) {
                simpleLowercaseMapping = code
            }
        }

        if (simpleTitleCaseMapping == null) {
            simpleTitleCaseMapping = simpleUppercaseMapping
        }

        unicodeData.push({
            code, name, generalCategory, canonicalCombiningClass, bidiClass,
            decompositionType, decompositionMapping, decimalDigit, digit, numeric,
            bidiMirrored, unicode1_0Name, isoComment, simpleUppercaseMapping,
            simpleLowercaseMapping, simpleTitleCaseMapping,
        })
    }

    return unicodeData
}

function readEastAsianWidths(eastAsianWidthContents) {
    const ranges = readHexRanges(eastAsianWidthContents)
    const eastAsianWidths = new Map(ranges.flatMap(range => {
        if (range.range1 == null) return [[ range.range0, range.description0 ]]
        return Array.from({ length: range.range1 - range.range0 + 1 },
                          (_, idx) => ([ idx + range.range0, range.description0 ]))
    }))

    return eastAsianWidths
}

function groupBy(elements, callbackKey, callbackMap) {
    const result = new Map()

    for (let idx = 0; idx < elements.length; ++idx) {
        const key = callbackKey(elements[idx])

        const collection = result.get(key);
        const mappedItem = callbackMap ? callbackMap(elements[idx]) : elements[idx]
        if (collection === undefined || collection === null) {
            result.set(key, [mappedItem]);
        }
        else {
            collection.push(mappedItem);
        }
    }

    return result
}

function reduceCasingMappings(mappings, fixedOutput = false) {
    for (let idx = 1; idx < mappings.length;) {
        const range0 = mappings[idx].inputEnd  - mappings[idx - 1].inputStart
        const range1 = mappings[idx].outputStart - mappings[idx - 1].outputStart

        if (mappings[idx].inputStart == mappings[idx - 1].inputEnd + 1 && (
            (fixedOutput && mappings[idx].outputStart == mappings[idx - 1].outputStart) || range0 == range1
        )) {
            mappings.splice(idx - 1, 2, {
                inputStart:  mappings[idx - 1].inputStart,
                inputEnd:    mappings[idx].inputEnd,
                outputStart: mappings[idx - 1].outputStart,
            })

            continue
        }

        ++idx
    }

    return mappings
}

function stringifyCaseMappings(mappings, defaultReturn = null, fixedOutput = false) {
    let result = ""

    const ranges = mappings.filter(e => e.inputStart != e.inputEnd)
    const values = mappings.filter(e => e.inputStart == e.inputEnd)

    if (values.length > 0) {
        const valueGroups = [ ... groupBy(values, e => e.outputStart).values() ]
        result += `    switch (codepoint) {\n`
        for (let idxGroup = 0; idxGroup < valueGroups.length; ++idxGroup) {
            if (valueGroups[idxGroup].length == 1) {
                const value = valueGroups[idxGroup][0]
                result += `    case ${value.inputStart}: return ${value.outputStart};\n`
                continue
            }

            let line = "    "
            const outputStart = valueGroups[idxGroup][0].outputStart
            for (let idxValue = 0; idxValue < valueGroups[idxGroup].length; ++idxValue) {
                const value = valueGroups[idxGroup][idxValue]
                const valueString = `case ${value.inputStart}: `
                const newLine = line + valueString

                if (newLine.length >= 80) {
                    result += line.trimEnd() + "\n"
                    line = `    ${valueString}`
                }
                else {
                    line = newLine
                }
            }

            if (line.length != 0) {
                result += line.trimEnd() + "\n"
            }

            result += `        return ${outputStart};\n`
        }
        result += `    }\n\n`

        // result += `    switch (codepoint) {\n`
        // for (let idx = 0; idx < values.length; ++idx) {
        //     result += `        case ${values[idx].inputStart}: return ${values[idx].outputStart};\n`
        // }
        // result += `    }\n\n`
    }

    if (ranges.length > 0) {
        for (let idx = 0; idx < ranges.length; ++idx) {
            result += `    if (codepoint > ${ranges[idx].inputStart - 1} && codepoint < ${ranges[idx].inputEnd + 1})`

            if (fixedOutput) {
                result += ` return ${ranges[idx].outputStart};\n`
            }
            else {
                const difference = ranges[idx].outputStart - ranges[idx].inputStart
                if      (difference > 0) result += ` return codepoint + ${ difference};\n`
                else if (difference < 0) result += ` return codepoint - ${-difference};\n`
                else                     result += ` return codepoint;\n`
            }
        }
    }

    if (defaultReturn != null) {
        result += `    return ${defaultReturn};\n`
    }

    return result
}

function getCharacterWidth(eastAsianWidths, codepoint, category) {
    let defaultWidth = 1

    switch (category) {
    case "Mc":
        defaultWidth = 0
        break
    case "Mn":
        if (codepoint == 0xAD) {
            return 1
        }
        /* fallthrough */
    case "Me":
    case "Cc":
    case "Cf":
    case "Cs":
    case "Zp":
        return 0
    }

    const characterClass = eastAsianWidths.get(codepoint)
    if (characterClass !== null && characterClass !== undefined) {
        const characterClassWidth = characterClassWidthMap.get(characterClass)

        if (characterClassWidth != null) {
            return characterClassWidth < 0 ? 1 : characterClassWidth
        }
    }

    return defaultWidth
}

async function main() {
    const filepath = process.argv[2] || "ignore/sfce_utf8.c"
    const UNICODE_BASE_URL = "https://www.unicode.org/Public/16.0.0/"

    console.log(`Loading files from url: ${UNICODE_BASE_URL}!`)
    const UNICODE_DATA = await getContentsFromURL(`${UNICODE_BASE_URL}/ucd/UnicodeData.txt`)
    const DERIVED_CORE_PROPERTIES = await getContentsFromURL(`${UNICODE_BASE_URL}/ucd/DerivedCoreProperties.txt`)
    const EAST_ASIAN_WIDTH = await getContentsFromURL(`${UNICODE_BASE_URL}/ucd/EastAsianWidth.txt`)

    const derivedCoreProperties = readHexRanges(DERIVED_CORE_PROPERTIES)

    const uppercase = createSetFromRanges(derivedCoreProperties.filter(e => e.description0 == "Uppercase"))
    const lowercase = createSetFromRanges(derivedCoreProperties.filter(e => e.description0 == "Lowercase"))

    const eastAsianWidths = readEastAsianWidths(EAST_ASIAN_WIDTH)
    const unicodeData = readUnicodeData(UNICODE_DATA, uppercase, lowercase)

    let uppercaseMappings = [], lowercaseMappings = [], categoryMappings = [], widthMappings = []

    for (let idx = 0; idx < unicodeData.length; ++idx) {
        if (unicodeData[idx].simpleUppercaseMapping != null) {
            uppercaseMappings.push({
                inputStart:  unicodeData[idx].code,
                inputEnd:    unicodeData[idx].code,
                outputStart: unicodeData[idx].simpleUppercaseMapping,
            })
        }

        if (unicodeData[idx].simpleLowercaseMapping != null) {
            lowercaseMappings.push({
                inputStart:  unicodeData[idx].code,
                inputEnd:    unicodeData[idx].code,
                outputStart: unicodeData[idx].simpleLowercaseMapping,
            })
        }

        categoryMappings.push({
            inputStart:  unicodeData[idx].code,
            inputEnd:    unicodeData[idx].code,
            outputStart: `SFCE_UNICODE_CATEGORY_${unicodeData[idx].generalCategory.toUpperCase()}`,
        })

        widthMappings.push({
            inputStart:  unicodeData[idx].code,
            inputEnd:    unicodeData[idx].code,
            outputStart: getCharacterWidth(eastAsianWidths, unicodeData[idx].code, unicodeData[idx].category),
        })
    }

    uppercaseMappings = reduceCasingMappings(uppercaseMappings, false)
    lowercaseMappings = reduceCasingMappings(lowercaseMappings, false)
    categoryMappings  = reduceCasingMappings(categoryMappings, true)
    widthMappings     = reduceCasingMappings(widthMappings, true)

    const stream = fs.createWriteStream(filepath)
    stream.write(`\
//
// Auto generated by ${path.basename(__filename)} at ${new Date().toLocaleDateString()}
// 

#include <stdint.h>

// https://www.compart.com/en/unicode/category
enum sfce_unicode_category {
    SFCE_UNICODE_CATEGORY_CN = 0, // Other, not assigned
    SFCE_UNICODE_CATEGORY_CC = 1, // Control
    SFCE_UNICODE_CATEGORY_CF = 2, // Format
    SFCE_UNICODE_CATEGORY_CO = 3, // Private Use
    SFCE_UNICODE_CATEGORY_CS = 4, // Surrogate
    SFCE_UNICODE_CATEGORY_LL = 5, // Lowercase Letter 2,15
    SFCE_UNICODE_CATEGORY_LM = 6, // Modifier Letter
    SFCE_UNICODE_CATEGORY_LO = 7, // Other Letter 127,00
    SFCE_UNICODE_CATEGORY_LT = 8, // Titlecase Letter
    SFCE_UNICODE_CATEGORY_LU = 9, // Uppercase Letter 1,79
    SFCE_UNICODE_CATEGORY_MC = 10, // Spacing Mark
    SFCE_UNICODE_CATEGORY_ME = 11, // Enclosing Mark
    SFCE_UNICODE_CATEGORY_MN = 12, // Nonspacing Mark  1,83
    SFCE_UNICODE_CATEGORY_ND = 13, // Decimal Number
    SFCE_UNICODE_CATEGORY_NL = 14, // Letter Number
    SFCE_UNICODE_CATEGORY_NO = 15, // Other Number
    SFCE_UNICODE_CATEGORY_PC = 16, // Connector Punctuation
    SFCE_UNICODE_CATEGORY_PD = 17, // Dash Punctuation
    SFCE_UNICODE_CATEGORY_PE = 18, // Close Punctuation
    SFCE_UNICODE_CATEGORY_PF = 19, // Final Punctuation
    SFCE_UNICODE_CATEGORY_PI = 20, // Initial Punctuation
    SFCE_UNICODE_CATEGORY_PO = 21, // Other Punctuation
    SFCE_UNICODE_CATEGORY_PS = 22, // Open Punctuation
    SFCE_UNICODE_CATEGORY_SC = 23, // Currency Symbol
    SFCE_UNICODE_CATEGORY_SK = 24, // Modifier Symbol
    SFCE_UNICODE_CATEGORY_SM = 25, // Math Symbol
    SFCE_UNICODE_CATEGORY_SO = 26, // Other Symbol 6,43
    SFCE_UNICODE_CATEGORY_ZL = 27, // Line Separator
    SFCE_UNICODE_CATEGORY_ZP = 28, // Paragraph Separator
    SFCE_UNICODE_CATEGORY_ZS = 29, // Space Separator
};
`)

    stream.write(`\n`)
    stream.write(`int32_t sfce_codepoint_to_upper(int32_t codepoint)\n{\n${stringifyCaseMappings(uppercaseMappings, "codepoint")}}\n`)
    stream.write(`\n`)
    stream.write(`int32_t sfce_codepoint_to_lower(int32_t codepoint)\n{\n${stringifyCaseMappings(lowercaseMappings, "codepoint")}}\n`)
    stream.write(`\n`)
    stream.write(`enum sfce_unicode_category sfce_codepoint_category(int32_t codepoint)\n{\n${stringifyCaseMappings(categoryMappings, "SFCE_UNICODE_CATEGORY_CN", true)}}\n`)
    stream.write(`\n`)
    stream.write(`int8_t sfce_codepoint_width(int32_t codepoint)\n{\n${stringifyCaseMappings(widthMappings, "1", true)}}\n`)
    // stream.write(`int32_t sfce_codepoint_to_lower(int32_t codepoint)\n`)

    // stream.write(`uint8_t sfce_codepoint_width(int32_t codepoint)\n`)
    
}

main()