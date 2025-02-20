// https://www.unicode.org/reports/tr44/#Name
// https://www.unicode.org/reports/tr44/tr44-34.html
// https://www.unicode.org/reports/tr11/

const fs = require("node:fs")
const path = require("node:path")
const https = require("node:https")

const PAGE_SIZE = 128

const unicodeLineRegex = /([0-9A-F]+);([^;]+);([A-Z]+);([0-9]+);([A-Z]+);(<([A-Z]*)>)?((\ ?[0-9A-F]+)*);([0-9]*);([0-9]*);([^;]*);([YN]*);([^;]*);([^;]*);([0-9A-F]*);([0-9A-F]*);([0-9A-F]*)$/i
const lineHexRangeRegex = /^([0-9A-F]+)(\.\.([0-9A-F]+))?\s*;\s*([^#]*)\s*([^\r\n]*)/i
const compositionExclusionsRegex = /^([0-9A-F]+)\s+#/i

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

    write(chunk, callback) {
        const requestedSize = this.#size + chunk.length
        if (requestedSize >= this.#buffer.length) {
            const temp = this.#buffer
            const capacity = roundNearestKiloByte(requestedSize)
            this.#buffer = new Uint8Array(capacity)
            this.#buffer.set(temp)
        }

        this.#buffer.set(chunk, this.#size)
        this.#size += chunk.length

        if (callback) {
            callback()
        }
    }

    buffer() { return Buffer.from(this.#buffer.subarray(0, this.#size)) }
    size() { return this.#size }
}

function toLines(string) {
    return string.split(/\r\n|[\r\n]/g)
}

function roundNearestKiloByte(value) {
    value = value | 0
    return (value + 0x0400 - 1) & -0x0400
}

function getContentsFromURL(url) {
    return new Promise((resolve, reject) => {
        const request = https.get(url, (response) => {
            const contents = new WritableByteStream()

            response.on("data", (chunk) => {
                contents.write(chunk)
            })

            response.on("end", () => {
                resolve(contents.buffer().toString())
            })
        })

        request.on("error", error => reject(error))
    })
}

function readUnicodeDataLine(line, uppercase, lowercase) {
    const matches = line.match(unicodeLineRegex)

    if (matches === null || matches === undefined) {
        return null
    }

    const result = {}

    result.code = matches[1] != null ? parseInt(matches[1], 16) : null
    result.name = matches[2]
    result.generalCategory = matches[3]
    result.canonicalCombiningClass = parseInt(matches[4])
    result.bidiClass = matches[5]
    result.decompositionType = matches[7]
    result.decompompositionMapping = matches[8] !== "" ? matches[8].split(/\s+/g).map(e => parseInt(e, 16)) : null
    result.decimalDigit = matches[10]
    result.digit = matches[11]
    result.numeric = matches[12]
    result.bidiMirrored = matches[13] == "Y"
    result.unicode1_0Name = matches[14]
    result.isoComment = matches[15]
    result.simpleUppercaseMapping = matches[16] !== "" ? parseInt(matches[16], 16) : null
    result.simpleLowercaseMapping = matches[17] !== "" ? parseInt(matches[17], 16) : null
    result.simpleTitlecaseMapping = matches[18] !== "" ? parseInt(matches[18], 16) : null

    const hasUppercaseMapping = result.simpleUppercaseMapping !== null || result.simpleUppercaseMapping !== undefined
    const hasLowercaseMapping = result.simpleLowercaseMapping !== null || result.simpleLowercaseMapping !== undefined

    if (!hasLowercaseMapping && !hasUppercaseMapping) {
        if (lowercase.has(code)) {
            result.simpleUppercaseMapping = result.code
        }

        if (uppercase.has(code)) {
            result.simpleLowercaseMapping = result.code
        }
    }

    if (result.simpleTitlecaseMapping == null) {
        result.simpleTitlecaseMapping = result.simpleUppercaseMapping
    }

    return result
}

function readHexRanges(content) {
    const result = []

    for (const line of toLines(content)) {
        const matches = line.match(lineHexRangeRegex)

        if (matches !== null && matches !== undefined) {
            const range0 = matches[1] != null ? parseInt(matches[1], 16) : null
            const range1 = matches[3] != null ? parseInt(matches[3], 16) : range0
            const description = matches[4] != null ? matches[4].trim() : null
            result.push({ range0, range1, description })
        }
    }

    return result
}

function readGraphemeBoundclasses(graphemeBreakContents, emojiDataContent) {
    const graphemeBoundclass = new Map()
    const hexRanges = [ ... readHexRanges(graphemeBreakContents),
                        ... readHexRanges(emojiDataContent) ]
    for (const {range0, range1, description} of hexRanges) {
        const newDescription = description.toUpperCase()
        for (let idx = range0; idx <= range1; ++idx) {
            graphemeBoundclass.set(idx, newDescription)
        }
    }

    return graphemeBoundclass
}

function readEastAsianWidths(eastAsianWidthContents) {
    const eastAsianWidths = new Map()
    for (const {range0, range1, description: characterClass } of readHexRanges(eastAsianWidthContents)) {
        if (range0 != null && range1 != null) {
            for (let idx = range0; idx <= range1; ++idx) {
                eastAsianWidths.set(idx, characterClass)
            }
        }
        else if (range0 != null) {
            eastAsianWidths.set(range0, characterClass)
        }
    }

    return eastAsianWidths
}

function readUnicodeData(unicodeDataContents, uppercase, lowercase) {
    const unicodeData = new Map()
    for (const line of toLines(unicodeDataContents)) {
        const data = readUnicodeDataLine(line, uppercase, lowercase)
        if (data !== null && data !== undefined) {
            unicodeData.set(data.code, data)
        }
    }

    return unicodeData
}

function readCaseFolding(contents) {
    const caseFolding = new Map()
    for (const line of toLines(contents)) {
        const matches = line.match(/^([0-9A-F]+); [CF]; ([0-9A-F ]+);/i)

        if (matches === null || matches === undefined) {
            continue
        }

        const code = parseInt(matches[1], 16)
        const sequence = matches[2].split(/\s+/).map(e => parseInt(e, 16))

        caseFolding.set(code, sequence)
    }

    return caseFolding
}

function readCompositionExclusions(contents) {
    const exclusions = []
    for (const line of toLines(contents)) {
        const matches = line.match(compositionExclusionsRegex)
        if (matches !== null && matches !== undefined) {
            exclusions.push(parseInt(matches[1], 16))
        }
    }

    return exclusions
}

function getCodepointByteLengthUTF8(codepoint) {
    if (codepoint < 0x00) {
        return 0;
    }

    if ((codepoint & 0xFFFFFF80) == 0) {
        return 1;
    }

    if ((codepoint & 0xFFFFF800) == 0) {
        return 2;
    }

    if ((codepoint & 0xFFFF0000) == 0) {
        return 3;
    }

    if ((codepoint & 0xFFE00000) == 0) {
        return 4;
    }

    return 0;
}

function getCharacterWidth(eastAsianWidths, codepoint, categroy) {
    let defaultWidth = 1

    switch (categroy) {
    case "Mc":
        defaultWidth = 0
        break
    case "Mn":
        if (codepoint == 0xAD) {
            return 1
        }
        /* falls through */
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

function writeIndexArray(stream, data, maximumLineSize) {
    const entryLength = getLargestElementLengthAsString(data) + 2
    const entriesPerLine = Math.max(1, Math.floor((maximumLineSize - 4) / entryLength))
    stream.write("{\n")

    let line = null
    done: for (let count = 0; count < data.length;) {
        line = `${data[count++]},`

        for (let idx = 1; idx < entriesPerLine; ++idx) {
            if (count >= data.length) {
                stream.write(`    ${line}\n`)
                break done
            }

            const remaining = line.length % entryLength
            if (remaining !== 0) {
                line = line.padEnd(line.length + entryLength - remaining)
            }

            line += `${data[count++]},`
        }

        stream.write(`    ${line}\n`)
    }

    stream.write("};\n")
}

function segmentArray(array, chunkSize) {
    const segments = []
    const segmentCount = Math.ceil(array.length / chunkSize)

    for (let idx = 0; idx < segmentCount; ++idx) {
        segments.push(array.slice(idx * chunkSize, (idx + 1) * chunkSize))
    }

    return segments
}

function compressAndCreateIndexArray(dataToCompress) {
    const compressedData = [], compressedIndices = [], processedData = new Map()
    
    for (let idx = 0; idx < dataToCompress.length; ++idx) {
        const data = dataToCompress[idx]
        const key = JSON.stringify(data)
        const index = processedData.get(key)

        if (index === null || index === undefined) {
            processedData.set(key, compressedData.length)
            compressedIndices.push(compressedData.length)
            compressedData.push(data)
        }
        else {
            compressedIndices.push(index)
        }
    }
    
    return [compressedData, compressedIndices]
}

function createPagedData(array, pageSize) {
    const [compressedData, indices] = compressAndCreateIndexArray(array);
    const [pages,          pageIndicies] = compressAndCreateIndexArray(segmentArray(indices, pageSize));
    return [ compressedData, pages, pageIndicies ]
}

function getLargestElementLengthAsString(array) {
    let length = 0
    
    for (let idx = 0; idx < array.length; ++idx) {
        length = Math.max(length, array[idx].toString().length)
    }

    return length
}

function createSetFromRanges(ranges) {
    const result = new Set()

    for (const { range0, range1 } of ranges) {
        for (let idx = range0; idx < range1; ++idx) {
            result.add(idx)
        }
    }

    return result
}

function createProgramUnicodeDescriptors(unicodeData, eastAsianWidths) {
    const descriptors = []

    for (let idx = 0; idx < unicodeData.length; ++idx) {
        const descriptor = {}
        const data = unicodeData[idx]

        descriptor.category = (data.generalCategory ?? "Cn").toUpperCase()
        descriptor.length = getCodepointByteLengthUTF8(data.code)
        descriptor.width = getCharacterWidth(eastAsianWidths, data.code, data.category)
        descriptor.combiningClass = data.canonicalCombiningClass
        descriptor.bidiClass = (data.bidiClass ?? "NONE").toUpperCase()
        descriptor.decomposition = (data.decompositionType ?? "NONE").toUpperCase()
        descriptor.decompositionMapping = data.decompompositionMapping ?? codepoint
        descriptor.uppercaseMapping = data.simpleUppercaseMapping ?? 0
        descriptor.lowercaseMapping = data.simpleLowercaseMapping ?? 0
        descriptor.titlecaseMapping = data.simpleTitlecaseMapping ?? 0
        descriptor.bidiMirrored = data.bidiMirrored ? 1 : 0
        
        descriptors.push(descriptor)
    }

    return descriptors
}

async function main() {
    const filepath = process.argv[2] || "sfce_utf8_properties.c"

    console.log("Loading files from url: \"https://www.unicode.org/Public/16.0.0/\"!")
    const UNICODE_DATA = await getContentsFromURL("https://www.unicode.org/Public/16.0.0/ucd/UnicodeData.txt")
    const GRAPHEME_BREAK_PROPERTY = await getContentsFromURL("https://www.unicode.org/Public/16.0.0/ucd/auxiliary/GraphemeBreakProperty.txt")
    const DERIVED_CORE_PROPERTIES = await getContentsFromURL("https://www.unicode.org/Public/16.0.0/ucd/DerivedCoreProperties.txt")
    const COMPOSITION_EXCLUSIONS = await getContentsFromURL("https://www.unicode.org/Public/16.0.0/ucd/CompositionExclusions.txt")
    const CASE_FOLDING = await getContentsFromURL("https://www.unicode.org/Public/16.0.0/ucd/CaseFolding.txt")
    const EAST_ASIAN_WIDTH = await getContentsFromURL("https://www.unicode.org/Public/16.0.0/ucd/EastAsianWidth.txt")
    const EMOJI_DATA = await getContentsFromURL("https://www.unicode.org/Public/16.0.0/ucd/emoji/emoji-data.txt")

    console.log("Parsing the information from the loaded files!")
    const derivedCoreProperties = readHexRanges(DERIVED_CORE_PROPERTIES)
    // const ignorable = createSetFromRanges(derivedCoreProperties.filter(e => e.description == "Default_Ignorable_Code_Point"))
    const uppercase = createSetFromRanges(derivedCoreProperties.filter(e => e.description == "Uppercase"))
    const lowercase = createSetFromRanges(derivedCoreProperties.filter(e => e.description == "Lowercase"))

    const graphemeBoundclasses = readGraphemeBoundclasses(GRAPHEME_BREAK_PROPERTY, EMOJI_DATA)
    const eastAsianWidths = readEastAsianWidths(EAST_ASIAN_WIDTH)
    const unicodeData = readUnicodeData(UNICODE_DATA, uppercase, lowercase)
    const caseFolding = readCaseFolding(CASE_FOLDING)
    const exclusions = readCompositionExclusions(COMPOSITION_EXCLUSIONS)

    console.log("Grouping unicode properties!")

    const propertyStrings = []

    for (let codepoint = 0; codepoint <= 0x10FFFF; ++codepoint) {
        const data = unicodeData.get(codepoint)
        
        const category = (data?.generalCategory ?? "Cn").toUpperCase()
        const length = data ? getCodepointByteLengthUTF8(data.code) : 0
        const width = data ? getCharacterWidth(eastAsianWidths, data.code, data.category) : 1
        const combiningClass = data?.canonicalCombiningClass ?? 0
        const bidiClass = (data?.bidiClass ?? "NONE").toUpperCase()
        const decomposition = (data?.decompositionType ?? "NONE").toUpperCase()
        // const decompositionMapping = data?.decompompositionMapping ?? codepoint
        const uppercaseMapping = data?.simpleUppercaseMapping ?? -1
        const lowercaseMapping = data?.simpleLowercaseMapping ?? -1
        const titlecaseMapping = data?.simpleTitlecaseMapping ?? -1
        const bidiMirrored = data?.bidiMirrored ? 1 : 0

        propertyStrings.push(`{ SFCE_UNICODE_CATEGORY_${category}, ${combiningClass}, SFCE_UNICODE_BIDI_CLASS_${bidiClass}, SFCE_UNICODE_DECOMPOSITION_${decomposition}, ${uppercaseMapping}, ${lowercaseMapping}, ${titlecaseMapping}, ${width}, ${length}, ${bidiMirrored} }`)
    }


    console.log(`Compressing ${propertyStrings.length} unicode properties!`)

    const [compressedData, pages, pageIndicies] = createPagedData(propertyStrings, PAGE_SIZE)
    const pageOffsets = pageIndicies.map(e => PAGE_SIZE * e)
    const indices     = pages.flat()

    const stream = fs.createWriteStream(filepath)

    stream.write(`\
//
// Auto generated by ${path.basename(__filename)} at ${new Date().toLocaleDateString()}
//
`)

    console.log(`Writing ${compressedData.length} unicode properties to "${filepath}"!`)

    stream.write(`static const struct sfce_utf8_property utf8_properties[${compressedData.length}] = `)
    writeIndexArray(stream, compressedData, 80)
 
    stream.write(`int32_t utf8_property_indices[${indices.length}] = `)
    writeIndexArray(stream, indices, 80)

    stream.write(`int32_t utf8_property_page_offsets[${pageOffsets.length}] = `)
    writeIndexArray(stream, pageOffsets, 80)

    stream.close()
}

main()
