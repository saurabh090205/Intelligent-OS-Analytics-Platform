const fs = require('fs');
const path = require('path');
const {
  Document,
  Packer,
  Paragraph,
  TextRun,
  HeadingLevel,
  Table,
  TableRow,
  TableCell,
  WidthType,
  BorderStyle,
  AlignmentType,
  ShadingType,
  Footer,
  Header,
  PageNumber,
  NumberFormat
} = require('docx');

async function generateReport() {
  console.log('Generating Academic Project Report DOCX...');

  // Styling Constants
  const FONT_PRIMARY = 'Calibri';
  const FONT_CODE = 'Consolas';
  const COLOR_NAVY = '1E3A8A';
  const COLOR_DARK = '1F2937';
  const COLOR_MUTED = '4B5563';
  const COLOR_CODE_BG = 'F3F4F6';
  const COLOR_TABLE_HEADER = '0F172A';

  // Helper Functions
  function createTitle(text) {
    return new Paragraph({
      alignment: AlignmentType.CENTER,
      spacing: { before: 240, after: 120 },
      children: [
        new TextRun({
          text,
          bold: true,
          size: 36, // 18pt
          font: FONT_PRIMARY,
          color: COLOR_NAVY,
        }),
      ],
    });
  }

  function createSubtitle(text) {
    return new Paragraph({
      alignment: AlignmentType.CENTER,
      spacing: { before: 60, after: 240 },
      children: [
        new TextRun({
          text,
          italics: true,
          size: 24, // 12pt
          font: FONT_PRIMARY,
          color: COLOR_MUTED,
        }),
      ],
    });
  }

  function createHeading1(text) {
    return new Paragraph({
      heading: HeadingLevel.HEADING_1,
      spacing: { before: 360, after: 140 },
      children: [
        new TextRun({
          text,
          bold: true,
          size: 30, // 15pt
          font: FONT_PRIMARY,
          color: COLOR_NAVY,
        }),
      ],
    });
  }

  function createHeading2(text) {
    return new Paragraph({
      heading: HeadingLevel.HEADING_2,
      spacing: { before: 240, after: 100 },
      children: [
        new TextRun({
          text,
          bold: true,
          size: 26, // 13pt
          font: FONT_PRIMARY,
          color: '0369A1',
        }),
      ],
    });
  }

  function createHeading3(text) {
    return new Paragraph({
      heading: HeadingLevel.HEADING_3,
      spacing: { before: 180, after: 80 },
      children: [
        new TextRun({
          text,
          bold: true,
          size: 24, // 12pt
          font: FONT_PRIMARY,
          color: COLOR_DARK,
        }),
      ],
    });
  }

  function createBody(text, options = {}) {
    return new Paragraph({
      spacing: { before: 60, after: 80, line: 276 }, // 1.15 line spacing
      alignment: options.alignment || AlignmentType.JUSTIFY,
      children: [
        new TextRun({
          text,
          size: 22, // 11pt
          font: FONT_PRIMARY,
          color: COLOR_DARK,
          bold: options.bold || false,
          italics: options.italics || false,
        }),
      ],
    });
  }

  function createBullet(text, boldPrefix = '') {
    const children = [];
    if (boldPrefix) {
      children.push(
        new TextRun({
          text: boldPrefix + ' ',
          bold: true,
          size: 22,
          font: FONT_PRIMARY,
          color: COLOR_DARK,
        })
      );
    }
    children.push(
      new TextRun({
        text,
        size: 22,
        font: FONT_PRIMARY,
        color: COLOR_DARK,
      })
    );
    return new Paragraph({
      bullet: { level: 0 },
      spacing: { before: 40, after: 40, line: 260 },
      children,
    });
  }

  function createCodeBlock(codeLines) {
    return codeLines.map((line) =>
      new Paragraph({
        spacing: { before: 20, after: 20, line: 220 },
        shading: { type: ShadingType.CLEAR, fill: COLOR_CODE_BG },
        children: [
          new TextRun({
            text: line || ' ',
            font: FONT_CODE,
            size: 19, // 9.5pt
            color: '1E293B',
          }),
        ],
      })
    );
  }

  function createBorderedCell(text, isHeader = false, widthPercent = 25) {
    return new TableCell({
      width: { size: widthPercent, type: WidthType.PERCENTAGE },
      shading: isHeader ? { type: ShadingType.CLEAR, fill: COLOR_TABLE_HEADER } : undefined,
      children: [
        new Paragraph({
          spacing: { before: 60, after: 60 },
          alignment: isHeader ? AlignmentType.CENTER : AlignmentType.LEFT,
          children: [
            new TextRun({
              text,
              bold: isHeader,
              size: isHeader ? 20 : 19,
              font: FONT_PRIMARY,
              color: isHeader ? 'FFFFFF' : COLOR_DARK,
            }),
          ],
        }),
      ],
    });
  }

  // Read the markdown content
  const mdPath = path.join(__dirname, '..', 'ACADEMIC_PROJECT_REPORT.md');
  const mdContent = fs.readFileSync(mdPath, 'utf8');
  const mdLines = mdContent.split('\n');

  const docChildren = [];

  // Parse lines into docx elements
  let inCodeBlock = false;
  let codeBuffer = [];

  for (let i = 0; i < mdLines.length; i++) {
    const line = mdLines[i];
    const trimmed = line.trim();

    if (trimmed.startsWith('```')) {
      if (inCodeBlock) {
        docChildren.push(...createCodeBlock(codeBuffer));
        codeBuffer = [];
        inCodeBlock = false;
      } else {
        inCodeBlock = true;
      }
      continue;
    }

    if (inCodeBlock) {
      codeBuffer.push(line);
      continue;
    }

    if (!trimmed) {
      continue;
    }

    // Horizontal Rule / Page Breaks
    if (trimmed === '---' || trimmed === '\\newpage') {
      docChildren.push(
        new Paragraph({
          spacing: { before: 120, after: 120 },
          pageBreakBefore: trimmed === '\\newpage',
          children: [],
        })
      );
      continue;
    }

    // Title / Headers
    if (trimmed.startsWith('# ')) {
      docChildren.push(createTitle(trimmed.replace('# ', '')));
      continue;
    }
    if (trimmed.startsWith('## ')) {
      docChildren.push(createHeading1(trimmed.replace('## ', '')));
      continue;
    }
    if (trimmed.startsWith('### ')) {
      docChildren.push(createHeading2(trimmed.replace('### ', '')));
      continue;
    }
    if (trimmed.startsWith('#### ')) {
      docChildren.push(createHeading3(trimmed.replace('#### ', '')));
      continue;
    }

    // Bullets
    if (trimmed.startsWith('* ') || trimmed.startsWith('- ')) {
      const rawText = trimmed.substring(2);
      // Check if bold prefix
      const boldMatch = rawText.match(/^\*\*(.*?)\*\*:?\s*(.*)$/);
      if (boldMatch) {
        docChildren.push(createBullet(boldMatch[2], boldMatch[1] + ':'));
      } else {
        docChildren.push(createBullet(rawText.replace(/\*\*/g, '')));
      }
      continue;
    }

    // Numbered list
    const numMatch = trimmed.match(/^(\d+)\.\s+(.*)$/);
    if (numMatch) {
      const rawText = numMatch[2];
      const boldMatch = rawText.match(/^\*\*(.*?)\*\*:?\s*(.*)$/);
      if (boldMatch) {
        docChildren.push(createBullet(boldMatch[2], `${numMatch[1]}. ${boldMatch[1]}:`));
      } else {
        docChildren.push(createBullet(rawText.replace(/\*\*/g, ''), `${numMatch[1]}.`));
      }
      continue;
    }

    // Table rows
    if (trimmed.startsWith('|') && trimmed.endsWith('|')) {
      // Collect whole table
      const tableLines = [trimmed];
      while (i + 1 < mdLines.length && mdLines[i + 1].trim().startsWith('|')) {
        i++;
        tableLines.push(mdLines[i].trim());
      }

      // Filter out separator row (e.g. |---|---|)
      const dataRows = tableLines.filter((l) => !l.match(/^\|[\s\-:|]+\|$/));
      if (dataRows.length > 0) {
        const rows = dataRows.map((r, rIdx) => {
          const cells = r
            .split('|')
            .slice(1, -1)
            .map((c) => c.trim().replace(/\*\*/g, ''));
          const widthPct = Math.floor(100 / (cells.length || 1));
          return new TableRow({
            children: cells.map((cellText) =>
              createBorderedCell(cellText, rIdx === 0, widthPct)
            ),
          });
        });

        docChildren.push(
          new Table({
            width: { size: 100, type: WidthType.PERCENTAGE },
            rows,
          })
        );
        docChildren.push(new Paragraph({ spacing: { after: 120 }, children: [] }));
      }
      continue;
    }

    // Subtitle check
    if (trimmed.startsWith('*Submitted in partial') || trimmed.startsWith('UNDER THE GUIDANCE')) {
      docChildren.push(createSubtitle(trimmed.replace(/\*/g, '')));
      continue;
    }

    // Normal body paragraph
    docChildren.push(createBody(trimmed.replace(/\*\*/g, '')));
  }

  const doc = new Document({
    sections: [
      {
        properties: {
          page: {
            margin: {
              top: 1440, // 1 inch (1440 dxa)
              right: 1440,
              bottom: 1440,
              left: 1440,
            },
          },
        },
        headers: {
          default: new Header({
            children: [
              new Paragraph({
                alignment: AlignmentType.RIGHT,
                children: [
                  new TextRun({
                    text: 'Intelligent OS Analytics Platform — Academic Capstone Report',
                    font: FONT_PRIMARY,
                    size: 16, // 8pt
                    color: '94A3B8',
                  }),
                ],
              }),
            ],
          }),
        },
        footers: {
          default: new Footer({
            children: [
              new Paragraph({
                alignment: AlignmentType.RIGHT,
                children: [
                  new TextRun({
                    text: 'Page ',
                    font: FONT_PRIMARY,
                    size: 18,
                    color: COLOR_MUTED,
                  }),
                  new TextRun({
                    children: [PageNumber.CURRENT],
                    font: FONT_PRIMARY,
                    size: 18,
                    color: COLOR_MUTED,
                  }),
                  new TextRun({
                    text: ' of ',
                    font: FONT_PRIMARY,
                    size: 18,
                    color: COLOR_MUTED,
                  }),
                  new TextRun({
                    children: [PageNumber.TOTAL_PAGES],
                    font: FONT_PRIMARY,
                    size: 18,
                    color: COLOR_MUTED,
                  }),
                ],
              }),
            ],
          }),
        },
        children: docChildren,
      },
    ],
  });

  const buffer = await Packer.toBuffer(doc);
  const outPath = path.join(__dirname, '..', 'ACADEMIC_PROJECT_REPORT.docx');
  fs.writeFileSync(outPath, buffer);
  console.log(`Report successfully written to ${outPath} (${(buffer.length / 1024).toFixed(1)} KB)`);
}

generateReport().catch((err) => {
  console.error('Failed to generate DOCX report:', err);
  process.exit(1);
});
