from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib.units import mm
from reportlab.lib import colors
from reportlab.platypus import (
    SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle,
    HRFlowable, PageBreak, KeepTogether
)
from reportlab.lib.enums import TA_LEFT, TA_CENTER, TA_JUSTIFY

W, H = A4
MARGIN = 20 * mm

# ── colour palette ────────────────────────────────────────────────────────────
C_BLACK   = colors.HexColor("#1a1a1a")
C_DARK    = colors.HexColor("#2c3e50")
C_ACCENT  = colors.HexColor("#e74c3c")
C_BLUE    = colors.HexColor("#2980b9")
C_GREEN   = colors.HexColor("#27ae60")
C_ORANGE  = colors.HexColor("#e67e22")
C_PURPLE  = colors.HexColor("#8e44ad")
C_GREY    = colors.HexColor("#7f8c8d")
C_LIGHT   = colors.HexColor("#ecf0f1")
C_WHITE   = colors.white
C_PANEL   = colors.HexColor("#f8f9fa")
C_BORDER  = colors.HexColor("#bdc3c7")

styles = getSampleStyleSheet()

def S(name, **kw):
    return ParagraphStyle(name, **kw)

# custom styles
TITLE_S = S("DocTitle",
    fontSize=28, textColor=C_ACCENT, spaceAfter=4,
    fontName="Helvetica-Bold", alignment=TA_CENTER)

SUBTITLE_S = S("DocSubtitle",
    fontSize=13, textColor=C_DARK, spaceAfter=20,
    fontName="Helvetica", alignment=TA_CENTER)

H1 = S("H1",
    fontSize=18, textColor=C_WHITE, spaceAfter=6, spaceBefore=18,
    fontName="Helvetica-Bold", backColor=C_DARK,
    borderPad=6, leftIndent=-4, rightIndent=-4,
    leading=24)

H2 = S("H2",
    fontSize=13, textColor=C_DARK, spaceAfter=4, spaceBefore=12,
    fontName="Helvetica-Bold",
    borderColor=C_ACCENT, borderWidth=0, leftIndent=0)

H3 = S("H3",
    fontSize=11, textColor=C_ACCENT, spaceAfter=3, spaceBefore=8,
    fontName="Helvetica-Bold")

BODY = S("Body",
    fontSize=9.5, textColor=C_BLACK, spaceAfter=5, spaceBefore=2,
    fontName="Helvetica", leading=14, alignment=TA_JUSTIFY)

CODE = S("Code",
    fontSize=8.5, textColor=C_DARK, spaceAfter=3, spaceBefore=3,
    fontName="Courier", leading=12, backColor=C_PANEL,
    leftIndent=8, rightIndent=4, borderPad=4)

BULLET = S("Bullet",
    fontSize=9.5, textColor=C_BLACK, spaceAfter=3,
    fontName="Helvetica", leading=13, leftIndent=14, bulletIndent=4)

NOTE = S("Note",
    fontSize=9, textColor=C_DARK, spaceAfter=4, spaceBefore=4,
    fontName="Helvetica-Oblique", leading=13,
    backColor=colors.HexColor("#fef9e7"), leftIndent=10, borderPad=5)

LABEL = S("Label",
    fontSize=9, textColor=C_WHITE, fontName="Helvetica-Bold",
    alignment=TA_CENTER)

def h1(text): return Paragraph(f"&nbsp; {text}", H1)
def h2(text): return Paragraph(text, H2)
def h3(text): return Paragraph(text, H3)
def body(text): return Paragraph(text, BODY)
def code(text): return Paragraph(text, CODE)
def bullet(text): return Paragraph(f"• &nbsp;{text}", BULLET)
def note(text): return Paragraph(f"<i>ℹ️  {text}</i>", NOTE)
def sp(n=6): return Spacer(1, n)
def hr(): return HRFlowable(width="100%", thickness=0.5, color=C_BORDER, spaceAfter=6, spaceBefore=6)

def badge_table(items_colors):
    """Inline badge row — [(text, bg_color), ...]"""
    data = [[Paragraph(f"<b>{t}</b>", LABEL) for t, _ in items_colors]]
    style = TableStyle([
        ('BACKGROUND', (i, 0), (i, 0), c) for i, (_, c) in enumerate(items_colors)
    ] + [
        ('TEXTCOLOR', (0, 0), (-1, -1), C_WHITE),
        ('FONTNAME',  (0, 0), (-1, -1), 'Helvetica-Bold'),
        ('FONTSIZE',  (0, 0), (-1, -1), 8),
        ('ROWBACKGROUNDS', (0, 0), (-1, -1), [None]),
        ('TOPPADDING',    (0, 0), (-1, -1), 4),
        ('BOTTOMPADDING', (0, 0), (-1, -1), 4),
        ('LEFTPADDING',   (0, 0), (-1, -1), 8),
        ('RIGHTPADDING',  (0, 0), (-1, -1), 8),
        ('ROUNDED',       (0, 0), (-1, -1), 3),
    ])
    return Table(data, style=style, hAlign='LEFT')

def stack_table(rows, title="GOAL STACK (top → bottom)"):
    """Renders a visual goal stack table."""
    header = [[Paragraph(f"<b>{title}</b>", S("SH",
        fontSize=8.5, fontName="Helvetica-Bold", textColor=C_WHITE, alignment=TA_CENTER))]]
    data = header + [[Paragraph(r, CODE)] for r in rows]
    n = len(data)
    style_cmds = [
        ('BACKGROUND', (0, 0), (-1, 0), C_DARK),
        ('TEXTCOLOR',  (0, 0), (-1, 0), C_WHITE),
        ('GRID',       (0, 0), (-1, -1), 0.4, C_BORDER),
        ('TOPPADDING',    (0, 0), (-1, -1), 4),
        ('BOTTOMPADDING', (0, 0), (-1, -1), 4),
        ('LEFTPADDING',   (0, 0), (-1, -1), 8),
        ('ROWBACKGROUNDS', (0, 1), (-1, -1), [C_PANEL, C_WHITE]),
    ]
    # highlight top (index 1 = first data row)
    if n > 1:
        style_cmds.append(('BACKGROUND', (0, 1), (-1, 1), colors.HexColor("#fff3cd")))
    t = Table(data, colWidths=[W - 2*MARGIN - 10], style=TableStyle(style_cmds))
    return t

def two_col(left_items, right_items, left_title="", right_title=""):
    ldata = ([[Paragraph(f"<b>{left_title}</b>", S("th", fontSize=9, fontName="Helvetica-Bold",
               textColor=C_WHITE, alignment=TA_CENTER))]] if left_title else []) + \
            [[i] for i in left_items]
    rdata = ([[Paragraph(f"<b>{right_title}</b>", S("th2", fontSize=9, fontName="Helvetica-Bold",
               textColor=C_WHITE, alignment=TA_CENTER))]] if right_title else []) + \
            [[i] for i in right_items]

    max_rows = max(len(ldata), len(rdata))
    while len(ldata) < max_rows: ldata.append([Paragraph("", BODY)])
    while len(rdata) < max_rows: rdata.append([Paragraph("", BODY)])

    merged = [[ldata[i][0], rdata[i][0]] for i in range(max_rows)]
    cw = (W - 2*MARGIN - 10) / 2
    style_cmds = [
        ('GRID',           (0, 0), (-1, -1), 0.3, C_BORDER),
        ('VALIGN',         (0, 0), (-1, -1), 'TOP'),
        ('TOPPADDING',     (0, 0), (-1, -1), 4),
        ('BOTTOMPADDING',  (0, 0), (-1, -1), 4),
        ('LEFTPADDING',    (0, 0), (-1, -1), 6),
    ]
    if left_title:
        style_cmds += [
            ('BACKGROUND', (0, 0), (0, 0), C_BLUE),
            ('BACKGROUND', (1, 0), (1, 0), C_ACCENT),
            ('TEXTCOLOR',  (0, 0), (-1, 0), C_WHITE),
        ]
    return Table(merged, colWidths=[cw, cw], style=TableStyle(style_cmds))

# ═══════════════════════════════════════════════════════════════════════════════
# BUILD DOCUMENT
# ═══════════════════════════════════════════════════════════════════════════════
story = []

# ── COVER ────────────────────────────────────────────────────────────────────
story += [
    Spacer(1, 40*mm),
    Paragraph("3D HEIST SIMULATOR", TITLE_S),
    Paragraph("AI Implementation Rule Book & SRS", SUBTITLE_S),
    HRFlowable(width="60%", thickness=2, color=C_ACCENT, spaceAfter=10, hAlign='CENTER'),
    Spacer(1, 6),
    Paragraph("Goal Stack Planning · STRIPS Operators · A* Heuristic Search", S("cov",
        fontSize=11, textColor=C_GREY, alignment=TA_CENTER, fontName="Helvetica-Oblique")),
    Spacer(1, 8*mm),
    Table([[
        Paragraph("Version 1.0", S("vt", fontSize=9, textColor=C_WHITE, fontName="Helvetica-Bold", alignment=TA_CENTER)),
        Paragraph("User = Robber · AI = Police", S("vt2", fontSize=9, textColor=C_WHITE, fontName="Helvetica-Bold", alignment=TA_CENTER)),
        Paragraph("Normal / Hard Mode", S("vt3", fontSize=9, textColor=C_WHITE, fontName="Helvetica-Bold", alignment=TA_CENTER)),
    ]], colWidths=[(W-2*MARGIN)/3]*3,
    style=TableStyle([
        ('BACKGROUND', (0,0),(0,0), C_BLUE),
        ('BACKGROUND', (1,0),(1,0), C_GREEN),
        ('BACKGROUND', (2,0),(2,0), C_ORANGE),
        ('TOPPADDING',(0,0),(-1,-1),6),('BOTTOMPADDING',(0,0),(-1,-1),6),
    ])),
    PageBreak(),
]

# ── 1. PROJECT OVERVIEW ───────────────────────────────────────────────────────
story += [
    h1("1. PROJECT OVERVIEW"),
    sp(),
    body("This document is the authoritative implementation reference for the 3D Heist Simulator AI system. "
         "It defines every rule, goal, operator, precondition, effect, and heuristic that must be implemented. "
         "Feed this entire document to any AI coding assistant before asking it to write or modify code."),
    sp(4),
    h2("1.1  What The Game Is"),
    body("A turn-based, adversarial 3D heist game rendered with Raylib. The human always plays the Robber. "
         "All Police units are AI-controlled. The game runs on a 3D grid (15×15 per floor, 1–3 floors). "
         "The AI uses STRIPS-style Goal Stack Planning as its primary decision engine, with A* (using "
         "goal-aware heuristics) as the movement executor."),
    sp(4),
    h2("1.2  Core Design Principles"),
    bullet("Every AI decision must trace back to a named Goal on the Goal Stack."),
    bullet("Every Goal is achieved by a STRIPS Operator with explicit Preconditions and Effects."),
    bullet("Every movement Operator calls A* with a GoalType-aware heuristic."),
    bullet("The Debug Window shows the live Goal Stack, active Operator, and A* successor log."),
    bullet("There is one Debug Window per Police AI agent, spawned at game start."),
    bullet("The Goal Stack is displayed exactly as solved in class: top-first, with compound goals using '^', "
           "completed goals struck through, cancelled goals in red."),
    sp(4),
    h2("1.3  Technology Stack"),
    Table([
        [Paragraph("<b>Component</b>", S("th",fontSize=9,fontName="Helvetica-Bold")),
         Paragraph("<b>Technology</b>", S("th",fontSize=9,fontName="Helvetica-Bold"))],
        [body("Rendering"),       body("Raylib (C++)")],
        [body("AI Planning"),     body("STRIPS Goal Stack (custom C++)")],
        [body("Pathfinding"),     body("A* on 3D grid with GoalType heuristic")],
        [body("Debug UI"),        body("Separate Raylib process per Police agent")],
        [body("Agent Comm"),      body("Snapshot JSON file per agent (agent_N_snapshot.txt)")],
    ], colWidths=[(W-2*MARGIN)*0.4, (W-2*MARGIN)*0.6],
    style=TableStyle([
        ('BACKGROUND',(0,0),(-1,0), C_DARK),('TEXTCOLOR',(0,0),(-1,0),C_WHITE),
        ('GRID',(0,0),(-1,-1),0.4,C_BORDER),
        ('ROWBACKGROUNDS',(0,1),(-1,-1),[C_PANEL,C_WHITE]),
        ('TOPPADDING',(0,0),(-1,-1),4),('BOTTOMPADDING',(0,0),(-1,-1),4),
        ('LEFTPADDING',(0,0),(-1,-1),6),
    ])),
    PageBreak(),
]

# ── 2. WORLD & MAP RULES ─────────────────────────────────────────────────────
story += [
    h1("2. WORLD & MAP RULES"),
    sp(),
    h2("2.1  Grid Layout"),
    bullet("15 × 15 cells per floor. Floors numbered 0 (ground) upward."),
    bullet("Difficulty determines number of floors: Easy=1, Normal=2, Hard=3."),
    bullet("Coordinates: Position(x, y, z) where z = floor number."),
    sp(4),
    h2("2.2  Cell Types"),
    Table([
        [Paragraph("<b>Cell Type</b>", S("th",fontSize=9,fontName="Helvetica-Bold")),
         Paragraph("<b>Symbol</b>", S("th",fontSize=9,fontName="Helvetica-Bold")),
         Paragraph("<b>Behaviour</b>", S("th",fontSize=9,fontName="Helvetica-Bold"))],
        [body("EMPTY"),       body("."), body("Walkable. No special effect.")],
        [body("WALL"),        body("#"), body("Impassable. Cannot be entered by any agent.")],
        [body("VAULT"),       body("V"), body("Robber collects vault by occupying this cell.")],
        [body("EXIT"),        body("E"), body("Robber escapes by occupying this cell after collecting vault.")],
        [body("STAIRS_UP"),   body("U"), body("Stepping on this cell moves agent up one floor automatically.")],
        [body("ELEVATOR_DN"), body("D"), body("Stepping on this cell moves agent down one floor automatically.")],
        [body("CCTV_ZONE"),   body("C"), body("Increases police heuristic awareness. Costs +2 for robber path.")],
        [body("ALERT_ZONE"),  body("A"), body("Triggers police boost (2 moves/turn for N turns). Consumed on trigger.")],
    ], colWidths=[(W-2*MARGIN)*0.25,(W-2*MARGIN)*0.1,(W-2*MARGIN)*0.65],
    style=TableStyle([
        ('BACKGROUND',(0,0),(-1,0),C_DARK),('TEXTCOLOR',(0,0),(-1,0),C_WHITE),
        ('GRID',(0,0),(-1,-1),0.4,C_BORDER),
        ('ROWBACKGROUNDS',(0,1),(-1,-1),[C_PANEL,C_WHITE]),
        ('TOPPADDING',(0,0),(-1,-1),4),('BOTTOMPADDING',(0,0),(-1,-1),4),
        ('LEFTPADDING',(0,0),(-1,-1),6),
    ])),
    sp(6),
    h2("2.3  Floor Transition Rules"),
    bullet("Robber: may use STAIRS_UP / ELEVATOR_DN freely at any time."),
    bullet("Police: may only use floor transitions if (vaultStolen == true) OR (alertBoostActive == true)."),
    bullet("Floor transition is automatic — stepping on a transition cell instantly moves agent to next floor."),
    note("Police on Floor 0 cannot chase Robber to Floor 1 until one of the two conditions above is met."),
    sp(4),
    h2("2.4  Alert Zone Mechanics"),
    bullet("When Robber steps on ALERT_ZONE: alertTriggered = true, cell converts to CCTV_ZONE (consumed)."),
    bullet("Police boost activates: policeBoostTurnsRemaining = 5 (difficulty-scaled). During boost police move 2 steps/turn."),
    bullet("RuleEngine.notifyAlertTriggered(pos) is called. The alertPos is stored for planner use."),
    bullet("When police reaches alertPos, respond(Police, Alert) goal is marked complete. Police then push protect(Police, Vault) back onto stack."),
    PageBreak(),
]

# ── 3. AGENTS ─────────────────────────────────────────────────────────────────
story += [
    h1("3. AGENTS"),
    sp(),
    h2("3.1  Robber (Human Controlled)"),
    bullet("Controlled via WASD keys. One move per turn."),
    bullet("Objective: collect vault, then reach exit."),
    bullet("Can step on ALERT_ZONE to distract police (intentional tactic)."),
    bullet("Floor transition: automatic on stepping onto U/D cell."),
    bullet("Win condition: Robber.pos == Exit.pos AND Robber.hasVault == true."),
    sp(6),
    h2("3.2  Police (AI Controlled — Multiple Agents)"),
    bullet("One Police agent per 'P' spawn marker in the map. Typically 1–3 agents."),
    bullet("Each Police agent has its own independent GoalStack and PoliceAIPlanner instance."),
    bullet("Each Police agent has its own Debug Window (separate process)."),
    bullet("Police are identified as Police1, Police2, Police3, etc. (1-indexed)."),
    bullet("Police agents do NOT block each other — they may occupy different cells."),
    bullet("Lose condition: any Police.pos == Robber.pos."),
    sp(6),
    h2("3.3  Police Activation Rules (CRITICAL)"),
    body("Police activation is floor-and-situation based. This is the most important behavioural rule:"),
    sp(4),
    Table([
        [Paragraph("<b>Situation</b>", S("th",fontSize=9,fontName="Helvetica-Bold")),
         Paragraph("<b>Which Police Activate</b>", S("th",fontSize=9,fontName="Helvetica-Bold")),
         Paragraph("<b>Goal Pushed</b>", S("th",fontSize=9,fontName="Helvetica-Bold"))],
        [body("Game starts"), body("None"), body("All push stayHalt(PoliceN)")],
        [body("Robber makes first move on Floor 0\n(Vault NOT on Floor 0)"),
         body("Police on Floor 0 only"),
         body("protect(Police1, Upstairs)")],
        [body("Robber makes first move on Floor 0\n(Vault IS on Floor 0)"),
         body("Police on Floor 0 only"),
         body("protect(Police1, Vault)")],
        [body("Robber reaches Floor 1\n(Vault on Floor 1)"),
         body("Police on Floor 1 only"),
         body("protect(Police2, Vault)")],
        [body("Alert zone triggered (any floor)"),
         body("Police on SAME FLOOR as alert"),
         body("respond(PoliceN, Alert)")],
        [body("Vault stolen"),
         body("ALL police, all floors"),
         body("catch(PoliceN, Robber) — each independently")],
    ], colWidths=[(W-2*MARGIN)*0.33,(W-2*MARGIN)*0.27,(W-2*MARGIN)*0.40],
    style=TableStyle([
        ('BACKGROUND',(0,0),(-1,0),C_DARK),('TEXTCOLOR',(0,0),(-1,0),C_WHITE),
        ('GRID',(0,0),(-1,-1),0.4,C_BORDER),
        ('ROWBACKGROUNDS',(0,1),(-1,-1),[C_PANEL,C_WHITE]),
        ('TOPPADDING',(0,0),(-1,-1),5),('BOTTOMPADDING',(0,0),(-1,-1),5),
        ('LEFTPADDING',(0,0),(-1,-1),6),('VALIGN',(0,0),(-1,-1),'TOP'),
    ])),
    sp(6),
    note("Police on Floor 0 do NOT follow Robber to Floor 1 unless vaultStolen OR alertBoostActive. "
         "When Robber moves between floors, each Police stays on its own floor and independently "
         "manages its own goal stack."),
    PageBreak(),
]

# ── 4. GOAL STACK SPECIFICATION ───────────────────────────────────────────────
story += [
    h1("4. GOAL STACK SPECIFICATION"),
    sp(),
    body("The Goal Stack is the primary AI decision mechanism. It follows the exact classroom STRIPS "
         "Goal Stack Planning model: goals and operators are pushed and popped from a single stack. "
         "The top of the stack is always the current focus. Compound goals use '^'. Operators are "
         "executable actions. Goals are conditions to be satisfied."),
    sp(6),
    h2("4.1  GoalEntry Structure"),
    code("struct GoalEntry {"),
    code("    string goalExpression;       // exact string — see Section 5 for all valid strings"),
    code("    bool   isOperator;           // true = executable action, false = goal/precondition"),
    code("    string operatorName;         // 'MOVE', 'STEAL', 'ESCAPE', 'CAPTURE', etc."),
    code("    vector<string> preconditions; // human-readable strings for debug display"),
    code("    vector<string> effects;       // human-readable strings for debug display"),
    code("    enum Status { PENDING, ACTIVE, PERFORMING, COMPLETED, CANCELLED } status;"),
    code("};"),
    sp(6),
    h2("4.2  Stack Display Rules (Debug Window)"),
    bullet("ACTIVE   → Yellow text. The current focus of the AI."),
    bullet("PENDING  → White text. Waiting below the active entry."),
    bullet("PERFORMING → Green text. Operator is mid-execution (A* running across turns)."),
    bullet("COMPLETED → Green text + strikethrough line drawn through text."),
    bullet("CANCELLED → Red text. Goal was interrupted."),
    bullet("Compound goals (contain '^') → Purple text."),
    bullet("Top-level/root goal (bottommost non-operator) → Red text."),
    sp(6),
    h2("4.3  GoalStack Operations"),
    Table([
        [Paragraph("<b>Method</b>", S("th",fontSize=9,fontName="Helvetica-Bold")),
         Paragraph("<b>Behaviour</b>", S("th",fontSize=9,fontName="Helvetica-Bold"))],
        [code("push(entry)"),        body("Demotes current ACTIVE to PENDING. Pushes new entry as ACTIVE (top = back of vector).")],
        [code("pop()"),              body("Removes top entry. Promotes next non-completed entry to ACTIVE.")],
        [code("peek()"),             body("Returns top entry without removing. top = back() of internal vector.")],
        [code("markComplete()"),     body("Sets top entry status to COMPLETED. Adds to completedGoals list.")],
        [code("markPerforming()"),   body("Sets top entry status to PERFORMING (operator running across turns).")],
        [code("finalizeCompleted()"),body("Pops all COMPLETED/CANCELLED entries from top. Promotes next ACTIVE.")],
        [code("cancel(expr)"),       body("Finds matching expression, marks CANCELLED, adds to cancelledGoals.")],
        [code("cancelAll()"),        body("Cancels entire stack. Used when vault stolen mid-other-goal.")],
        [code("getStack()"),         body("Returns entries top-first (index 0 = top). Used by snapshot writer.")],
    ], colWidths=[(W-2*MARGIN)*0.35,(W-2*MARGIN)*0.65],
    style=TableStyle([
        ('BACKGROUND',(0,0),(-1,0),C_DARK),('TEXTCOLOR',(0,0),(-1,0),C_WHITE),
        ('GRID',(0,0),(-1,-1),0.4,C_BORDER),
        ('ROWBACKGROUNDS',(0,1),(-1,-1),[C_PANEL,C_WHITE]),
        ('TOPPADDING',(0,0),(-1,-1),4),('BOTTOMPADDING',(0,0),(-1,-1),4),
        ('LEFTPADDING',(0,0),(-1,-1),6),('VALIGN',(0,0),(-1,-1),'TOP'),
    ])),
    PageBreak(),
]

# ── 5. GOAL EXPRESSIONS (exact strings) ──────────────────────────────────────
story += [
    h1("5. EXACT GOAL EXPRESSION STRINGS"),
    sp(),
    body("These are the ONLY valid goalExpression strings. Use them verbatim everywhere — "
         "in GoalEntry construction, snapshot JSON, string comparisons, and debug display. "
         "Any deviation will break the planner logic."),
    sp(6),
    h2("5.1  Police Goal Expressions"),
    Table([
        [Paragraph("<b>Situation</b>", S("th",fontSize=9,fontName="Helvetica-Bold")),
         Paragraph("<b>goalExpression string</b>", S("th",fontSize=9,fontName="Helvetica-Bold")),
         Paragraph("<b>isOp</b>", S("th",fontSize=9,fontName="Helvetica-Bold"))],
        [body("Initial / waiting"),           code('"stayHalt(PoliceN)"'),          body("false")],
        [body("Protect when vault on floor"),  code('"protect(PoliceN, Vault)"'),    body("false")],
        [body("Protect precondition"),         code('"sameFloor(PoliceN, Vault) ^ pathAvailable(PoliceN, Vault)"'), body("false")],
        [body("Protect operator"),             code('"MOVE(PoliceN, Vault)"'),       body("true")],
        [body("Protect when vault NOT on floor"),code('"protect(PoliceN, Upstairs)"'),body("false")],
        [body("Floor move precond"),           code('"sameFloor(PoliceN, Upstairs) ^ pathAvailable(PoliceN, Upstairs)"'),body("false")],
        [body("Floor move operator (up)"),     code('"MOVE(PoliceN, Upstairs)"'),    body("true")],
        [body("Floor move operator (down)"),   code('"MOVE(PoliceN, Downstairs)"'),  body("true")],
        [body("Floor move precond"),           code('"onTransitionCell(PoliceN)"'),  body("false")],
        [body("Alert response goal"),          code('"respond(PoliceN, Alert)"'),    body("false")],
        [body("Alert precondition"),           code('"sameFloor(PoliceN, Alert) ^ pathAvailable(PoliceN, Alert)"'),body("false")],
        [body("Alert operator"),               code('"MOVE(PoliceN, Alert)"'),       body("true")],
        [body("Chase goal"),                   code('"catch(PoliceN, Robber)"'),     body("false")],
        [body("Chase precondition"),           code('"sameFloor(PoliceN, Robber) ^ pathAvailable(PoliceN, Robber)"'),body("false")],
        [body("Chase operator"),               code('"MOVE(PoliceN, Robber)"'),      body("true")],
        [body("Capture operator"),             code('"CAPTURE(PoliceN, Robber)"'),   body("true")],
    ], colWidths=[(W-2*MARGIN)*0.32,(W-2*MARGIN)*0.50,(W-2*MARGIN)*0.18],
    style=TableStyle([
        ('BACKGROUND',(0,0),(-1,0),C_DARK),('TEXTCOLOR',(0,0),(-1,0),C_WHITE),
        ('GRID',(0,0),(-1,-1),0.4,C_BORDER),
        ('ROWBACKGROUNDS',(0,1),(-1,-1),[C_PANEL,C_WHITE]),
        ('TOPPADDING',(0,0),(-1,-1),4),('BOTTOMPADDING',(0,0),(-1,-1),4),
        ('LEFTPADDING',(0,0),(-1,-1),6),('VALIGN',(0,0),(-1,-1),'TOP'),
        ('FONTSIZE',(0,0),(-1,-1),8.5),
    ])),
    note("Replace N with the 1-based police index: Police1, Police2, Police3. "
         "This makes each agent's stack uniquely identifiable in the debug window."),
    sp(8),
    h2("5.2  Robber Goal Expressions (AI Robber mode only)"),
    Table([
        [Paragraph("<b>Situation</b>", S("th",fontSize=9,fontName="Helvetica-Bold")),
         Paragraph("<b>goalExpression string</b>", S("th",fontSize=9,fontName="Helvetica-Bold")),
         Paragraph("<b>isOp</b>", S("th",fontSize=9,fontName="Helvetica-Bold"))],
        [body("Root compound goal"),           code('"steal(Robber, Vault) ^ escape(Robber, Exit)"'),body("false")],
        [body("Steal sub-goal"),               code('"steal(Robber, Vault)"'),       body("false")],
        [body("Steal precondition"),           code('"atVault(Robber) ^ vaultClear(Vault)"'),       body("false")],
        [body("Steal operator"),               code('"STEAL(Robber, Vault)"'),       body("true")],
        [body("Move to vault operator"),       code('"MOVE(Robber, Vault)"'),        body("true")],
        [body("Vault move precond"),           code('"sameFloor(Robber, Vault) ^ pathAvailable(Robber, Vault)"'),body("false")],
        [body("Distract goal"),                code('"distract(Robber, Alert)"'),    body("false")],
        [body("Distract precond"),             code('"sameFloor(Robber, AlertZone) ^ pathAvailable(Robber, AlertZone)"'),body("false")],
        [body("Distract operator"),            code('"MOVE(Robber, AlertZone)"'),    body("true")],
        [body("Escape sub-goal"),              code('"escape(Robber, Exit)"'),       body("false")],
        [body("Escape precondition"),          code('"atExit(Robber) ^ hasVault(Robber)"'),         body("false")],
        [body("Escape operator"),              code('"ESCAPE(Robber)"'),             body("true")],
        [body("Move to exit operator"),        code('"MOVE(Robber, Exit)"'),         body("true")],
        [body("Exit move precond"),            code('"sameFloor(Robber, Exit) ^ pathAvailable(Robber, Exit)"'),body("false")],
        [body("Floor up operator"),            code('"MOVE(Robber, Upstairs)"'),     body("true")],
        [body("Floor down operator"),          code('"MOVE(Robber, Downstairs)"'),   body("true")],
        [body("Evade goal"),                   code('"evade(Robber, Police)"'),      body("false")],
        [body("Evade precond"),                code('"pathAvailable(Robber, SafeCell) ^ maxDistPolice(Robber)"'),body("false")],
        [body("Evade operator"),               code('"MOVE(Robber, SafeCell)"'),     body("true")],
    ], colWidths=[(W-2*MARGIN)*0.32,(W-2*MARGIN)*0.50,(W-2*MARGIN)*0.18],
    style=TableStyle([
        ('BACKGROUND',(0,0),(-1,0),C_DARK),('TEXTCOLOR',(0,0),(-1,0),C_WHITE),
        ('GRID',(0,0),(-1,-1),0.4,C_BORDER),
        ('ROWBACKGROUNDS',(0,1),(-1,-1),[C_PANEL,C_WHITE]),
        ('TOPPADDING',(0,0),(-1,-1),4),('BOTTOMPADDING',(0,0),(-1,-1),4),
        ('LEFTPADDING',(0,0),(-1,-1),6),('VALIGN',(0,0),(-1,-1),'TOP'),
        ('FONTSIZE',(0,0),(-1,-1),8.5),
    ])),
    PageBreak(),
]

# ── 6. POLICE GOAL STACK WALKTHROUGH ─────────────────────────────────────────
story += [
    h1("6. POLICE GOAL STACK — FULL WALKTHROUGH"),
    sp(),
    h2("6.1  Game Start"),
    body("All Police push stayHalt. Stack is identical for every Police agent at start:"),
    sp(4),
    stack_table([
        "stayHalt(Police1)          [ACTIVE]   ← top",
    ], "Police1 STACK at game start"),
    sp(8),
    h2("6.2  Robber Makes First Move — Vault NOT On This Floor"),
    body("Robber moves on Floor 0. Vault is on Floor 1. Police1 is on Floor 0."),
    body("Police1 detects: robberMadeFirstMoveOnFloor[0] = true. Vault.floor != Police.floor."),
    body("stayHalt is marked COMPLETE. protect(Police1, Upstairs) block is pushed:"),
    sp(4),
    stack_table([
        "MOVE(Police1, Upstairs)                          [ACTIVE]   ← top",
        "sameFloor(Police1, Upstairs) ^ pathAvailable(Police1, Upstairs)  [PENDING]",
        "protect(Police1, Upstairs)                       [PENDING]",
    ], "Police1 STACK — Robber on Floor 0, Vault on Floor 1"),
    sp(4),
    body("MOVE(Police1, Upstairs) executes: A* finds nearest STAIRS_UP cell. Police1 moves toward it. "
         "Once on transition cell, floor changes. Goal marks COMPLETE."),
    sp(8),
    h2("6.3  Robber Makes First Move — Vault ON This Floor"),
    body("Robber moves on Floor 1. Vault is also on Floor 1. Police2 is on Floor 1."),
    body("Police2 detects: robberMadeFirstMoveOnFloor[1] = true. Vault.floor == Police.floor."),
    body("stayHalt is marked COMPLETE. protect(Police2, Vault) block is pushed:"),
    sp(4),
    stack_table([
        "MOVE(Police2, Vault)                             [ACTIVE]   ← top",
        "sameFloor(Police2, Vault) ^ pathAvailable(Police2, Vault)  [PENDING]",
        "protect(Police2, Vault)                          [PENDING]",
    ], "Police2 STACK — Robber on Floor 1, Vault on Floor 1"),
    sp(8),
    h2("6.4  Alert Zone Triggered (Police mid-protect)"),
    body("Robber steps on ALERT_ZONE on Floor 1. respond block is pushed ON TOP without cancelling protect:"),
    sp(4),
    stack_table([
        "MOVE(Police2, Alert)                             [ACTIVE]   ← top",
        "sameFloor(Police2, Alert) ^ pathAvailable(Police2, Alert)  [PENDING]",
        "respond(Police2, Alert)                          [PENDING]",
        "── protect block below (intact, not cancelled) ──────────────────",
        "MOVE(Police2, Vault)                             [PENDING]",
        "sameFloor(Police2, Vault) ^ pathAvailable(Police2, Vault)  [PENDING]",
        "protect(Police2, Vault)                          [PENDING]",
    ], "Police2 STACK — Alert triggered during protect"),
    sp(4),
    body("Police2 moves toward Alert. When Police2.pos == alertPos:"),
    bullet("respond(Police2, Alert) marked COMPLETE."),
    bullet("respond block is finalized (popped)."),
    bullet("protect(Police2, Vault) block resumes as ACTIVE — police returns to protecting vault."),
    PageBreak(),
]

story += [
    h2("6.5  Vault Stolen Mid-Alert-Response (INTERRUPT — Critical)"),
    body("Police2 is moving toward Alert (MOVE(Police2, Alert) is PERFORMING). "
         "Robber reaches vault. vaultJustStolen = true."),
    body("Interrupt fires at TOP of runTurn(), BEFORE executeTop():"),
    sp(4),
    body("<b>Step 1:</b> cancelAll() — entire stack is cancelled:"),
    stack_table([
        "MOVE(Police2, Alert)        [CANCELLED]",
        "sameFloor ^ pathAvail...    [CANCELLED]",
        "respond(Police2, Alert)     [CANCELLED]",
        "MOVE(Police2, Vault)        [CANCELLED]",
        "sameFloor ^ pathAvail...    [CANCELLED]",
        "protect(Police2, Vault)     [CANCELLED]",
    ], "Police2 STACK — immediately after cancelAll()"),
    sp(4),
    body("<b>Step 2:</b> pushCatchBlock(world) — catch block pushed:"),
    stack_table([
        "MOVE(Police2, Robber)                            [ACTIVE]   ← top",
        "sameFloor(Police2, Robber) ^ pathAvailable(Police2, Robber)  [PENDING]",
        "catch(Police2, Robber)                           [PENDING]",
    ], "Police2 STACK — after vault stolen interrupt"),
    sp(4),
    note("Stack insertion order for catch block — push in this exact sequence so top is MOVE: "
         "(1) push catch(Police2, Robber)  "
         "(2) push sameFloor ^ pathAvailable compound  "
         "(3) push MOVE(Police2, Robber)   ← this becomes top"),
    sp(8),
    h2("6.6  Vault Stolen — Floor Transition Needed (Multi-Floor Chase)"),
    body("Police1 is on Floor 0. Robber is on Floor 1. Vault stolen. Police1 needs to chase."),
    stack_table([
        "MOVE(Police1, Upstairs)                          [ACTIVE]   ← top",
        "onTransitionCell(Police1)                        [PENDING]",
        "sameFloor(Police1, Robber) ^ pathAvailable(Police1, Robber)  [PENDING]",
        "MOVE(Police1, Robber)                            [PENDING]",
        "catch(Police1, Robber)                           [PENDING]",
    ], "Police1 STACK — vault stolen, Robber on different floor"),
    sp(4),
    body("Stack insertion order when floors differ — push in this sequence:"),
    code("(1) push catch(Police1, Robber)"),
    code("(2) push sameFloor(Police1, Robber) ^ pathAvailable(Police1, Robber)"),
    code("(3) push MOVE(Police1, Robber)"),
    code("(4) push onTransitionCell(Police1)          // floor precond"),
    code("(5) push MOVE(Police1, Upstairs/Downstairs) // this is now top"),
    sp(4),
    note("MOVE(Police1, Robber) must be RE-PLANNED every turn since Robber moves. "
         "When MOVE(Police1, Robber) becomes ACTIVE again, call A* fresh with current Robber pos."),
    sp(8),
    h2("6.7  Police2 Chase — Same Floor (Simpler Stack)"),
    body("Police2 is on Floor 1. Robber is also on Floor 1. Vault stolen."),
    body("sameFloor precondition is already satisfied — no floor move needed:"),
    stack_table([
        "MOVE(Police2, Robber)                            [ACTIVE]   ← top",
        "sameFloor(Police2, Robber) ^ pathAvailable(Police2, Robber)  [PENDING]",
        "catch(Police2, Robber)                           [PENDING]",
    ], "Police2 STACK — vault stolen, same floor as Robber"),
    PageBreak(),
]

# ── 7. STRIPS OPERATORS ───────────────────────────────────────────────────────
story += [
    h1("7. STRIPS OPERATORS — COMPLETE DEFINITION"),
    sp(),
    body("Every operator has: Name, Preconditions (must be true before execution), "
         "Effects (what changes after execution), and Execution method."),
    sp(6),
]

ops = [
    ("MOVE(Agent, Target)",
     ["pathExists(Agent.pos, Target)", "NOT wall(Target)", "Agent.pos is valid"],
     ["Agent.pos = next step on A* path toward Target"],
     "Call A*(Agent.pos, Target) with GoalType heuristic. Execute first step only. "
     "If Agent.pos == Target after step: markComplete(). Else: markPerforming() (continues next turn)."),
    ("MOVE_FLOOR(Agent, TargetFloor) — via MOVE to Upstairs/Downstairs",
     ["transitionCellExists on Agent.currentFloor",
      "IF Agent==Police: vaultStolen==true OR alertBoostActive==true"],
     ["Agent.floor = TargetFloor (automatic on stepping transition cell)"],
     "Find nearest STAIRS_UP or ELEVATOR_DN on current floor. "
     "Call MOVE(Agent, transitionCell). Step onto it triggers auto floor change."),
    ("STEAL(Robber, Vault)",
     ["Robber.pos == Vault.pos", "vaultStolen == false"],
     ["vaultStolen = true", "Robber.hasVault = true"],
     "Direct state update. No A* needed. Triggers vaultJustStolen flag for police interrupt."),
    ("ESCAPE(Robber)",
     ["Robber.pos == Exit.pos", "Robber.hasVault == true"],
     ["robberWon = true", "gameOver = true"],
     "Direct state update. Triggers game-over ROBBER_WON."),
    ("CAPTURE(PoliceN, Robber)",
     ["Police.pos == Robber.pos"],
     ["policeWon = true", "gameOver = true"],
     "Direct state update. Triggers game-over POLICE_WON."),
    ("DISTRACT(Robber, AlertZone)",
     ["alertZoneExists on Robber.currentFloor", "NOT Robber.pos == AlertZonePos"],
     ["alertTriggered = true (when stepped)"],
     "Robber intentionally steps on ALERT_ZONE. Indirect — triggered by world state update "
     "in GameEngineGUI when Robber.pos matches any ALERT_ZONE cell."),
    ("STAY_HALT(PoliceN)",
     ["always true"],
     ["no change"],
     "Police does nothing. Returns current pos. Stack entry stays ACTIVE until robber moves."),
]

for name, preconds, effects, execution in ops:
    story.append(KeepTogether([
        h3(f"► {name}"),
        Table([
            [Paragraph("<b>Preconditions</b>", S("l",fontSize=9,fontName="Helvetica-Bold",textColor=C_WHITE)),
             Paragraph("<b>Effects</b>", S("l",fontSize=9,fontName="Helvetica-Bold",textColor=C_WHITE)),
             Paragraph("<b>Execution</b>", S("l",fontSize=9,fontName="Helvetica-Bold",textColor=C_WHITE))],
            [
                "\n".join([f"• {p}" for p in preconds]),
                "\n".join([f"→ {e}" for e in effects]),
                execution
            ]
        ], colWidths=[(W-2*MARGIN)*0.30,(W-2*MARGIN)*0.28,(W-2*MARGIN)*0.42],
        style=TableStyle([
            ('BACKGROUND',(0,0),(-1,0),C_BLUE),('TEXTCOLOR',(0,0),(-1,0),C_WHITE),
            ('GRID',(0,0),(-1,-1),0.4,C_BORDER),
            ('BACKGROUND',(0,1),(-1,1),C_PANEL),
            ('TOPPADDING',(0,0),(-1,-1),5),('BOTTOMPADDING',(0,0),(-1,-1),5),
            ('LEFTPADDING',(0,0),(-1,-1),6),('VALIGN',(0,0),(-1,-1),'TOP'),
            ('FONTSIZE',(0,1),(-1,-1),8.5),
        ])),
        sp(6),
    ]))

story.append(PageBreak())

# ── 8. INTERRUPT PRIORITY SYSTEM ─────────────────────────────────────────────
story += [
    h1("8. INTERRUPT PRIORITY SYSTEM"),
    sp(),
    body("Interrupts are checked at the VERY START of every Police agent's runTurn(), "
         "before any goal decomposition or operator execution. They override whatever the "
         "police was doing. Order of priority:"),
    sp(6),
    Table([
        [Paragraph("<b>Priority</b>", S("th",fontSize=9,fontName="Helvetica-Bold")),
         Paragraph("<b>Trigger Condition</b>", S("th",fontSize=9,fontName="Helvetica-Bold")),
         Paragraph("<b>Action</b>", S("th",fontSize=9,fontName="Helvetica-Bold")),
         Paragraph("<b>Stack Operation</b>", S("th",fontSize=9,fontName="Helvetica-Bold"))],
        [body("1 (HIGHEST)"), body("vaultJustStolen == true\nAND catch not already on stack"),
         body("Cancel everything. Chase robber."),
         body("cancelAll()\npushCatchBlock(world)")],
        [body("2"), body("alertJustTriggered == true\nAND respond not already on stack\nAND police on same floor as alert"),
         body("Push respond on top. Keep protect below."),
         body("pushRespondBlock(world)\n(no cancel)")],
        [body("3 (LOWEST)"), body("robberMadeFirstMoveOnFloor\nAND stayHalt is active"),
         body("Replace stayHalt with appropriate protect goal."),
         body("markComplete(stayHalt)\npushProtectBlock(world)")],
    ], colWidths=[(W-2*MARGIN)*0.1,(W-2*MARGIN)*0.30,(W-2*MARGIN)*0.28,(W-2*MARGIN)*0.32],
    style=TableStyle([
        ('BACKGROUND',(0,0),(-1,0),C_DARK),('TEXTCOLOR',(0,0),(-1,0),C_WHITE),
        ('GRID',(0,0),(-1,-1),0.4,C_BORDER),
        ('ROWBACKGROUNDS',(0,1),(-1,-1),[colors.HexColor("#fdecea"),C_PANEL,C_WHITE]),
        ('TOPPADDING',(0,0),(-1,-1),5),('BOTTOMPADDING',(0,0),(-1,-1),5),
        ('LEFTPADDING',(0,0),(-1,-1),6),('VALIGN',(0,0),(-1,-1),'TOP'),
        ('FONTSIZE',(0,1),(-1,-1),8.5),
    ])),
    sp(6),
    note("vaultJustStolen = (world.vaultStolen == true AND lastVaultStolen == false). "
         "alertJustTriggered = (world.alertTriggered == true AND lastAlertTriggered == false). "
         "Always update lastVaultStolen and lastAlertTriggered at end of handleInterrupts()."),
    sp(8),
    h2("8.1  runTurn() Execution Order"),
    body("Every Police agent's runTurn() must follow this exact sequence every turn:"),
    sp(4),
    Table([
        [Paragraph("<b>Step</b>", S("th",fontSize=9,fontName="Helvetica-Bold")),
         Paragraph("<b>Action</b>", S("th",fontSize=9,fontName="Helvetica-Bold")),
         Paragraph("<b>Code</b>", S("th",fontSize=9,fontName="Helvetica-Bold"))],
        [body("1"), body("Initialize if first turn"), code("ensureInitialStack(world)")],
        [body("2"), body("Clean up completed/cancelled from top"), code("goalStack.finalizeCompleted()")],
        [body("3"), body("Check interrupts"), code("handleInterrupts(world)")],
        [body("4"), body("Execute top of stack (ONE move)"), code("executeTop(world)")],
        [body("5"), body("Return new position"), code("return world.policePos")],
    ], colWidths=[(W-2*MARGIN)*0.08,(W-2*MARGIN)*0.38,(W-2*MARGIN)*0.54],
    style=TableStyle([
        ('BACKGROUND',(0,0),(-1,0),C_DARK),('TEXTCOLOR',(0,0),(-1,0),C_WHITE),
        ('GRID',(0,0),(-1,-1),0.4,C_BORDER),
        ('ROWBACKGROUNDS',(0,1),(-1,-1),[C_PANEL,C_WHITE]),
        ('TOPPADDING',(0,0),(-1,-1),5),('BOTTOMPADDING',(0,0),(-1,-1),5),
        ('LEFTPADDING',(0,0),(-1,-1),6),
    ])),
    PageBreak(),
]

# ── 9. HEURISTIC ENGINE ───────────────────────────────────────────────────────
story += [
    h1("9. HEURISTIC ENGINE — A* GOAL-AWARE HEURISTICS"),
    sp(),
    body("A* is called by planners to execute MOVE operators. The heuristic function takes a GoalType "
         "parameter so it optimises for the current goal, not a generic distance. All heuristics use "
         "Manhattan distance as the base. Difficulty adjusts weights."),
    sp(6),
    h2("9.1  GoalType Enum"),
    code("enum GoalType {"),
    code("    NONE, PROTECT_VAULT, CHASE_ROBBER, RESPOND_ALERT,"),
    code("    REACH_VAULT, ESCAPE_TO_EXIT, REACH_ALERT_ZONE, MOVE_FLOOR_GOAL"),
    code("};"),
    sp(6),
    h2("9.2  Heuristic Formulas"),
    Table([
        [Paragraph("<b>GoalType</b>", S("th",fontSize=9,fontName="Helvetica-Bold")),
         Paragraph("<b>Formula</b>", S("th",fontSize=9,fontName="Helvetica-Bold")),
         Paragraph("<b>Used by</b>", S("th",fontSize=9,fontName="Helvetica-Bold"))],
        [code("PROTECT_VAULT"),
         body("w1*manhattan(n, VaultPos) + w2*(1/(distToRobber+1)) + w3*cctvBonus(n) + w4*alertZonePenalty(n)"),
         body("Police — protect/respond-after")],
        [code("CHASE_ROBBER"),
         body("w1*manhattan(n, RobberPos) + w2*floorDiffPenalty(n, Robber) + w3*transitionBonus(n)"),
         body("Police — catch")],
        [code("RESPOND_ALERT"),
         body("w1*manhattan(n, AlertPos) + w2*vaultExposurePenalty(n, VaultPos)"),
         body("Police — respond")],
        [code("MOVE_FLOOR_GOAL"),
         body("w1*manhattan(n, nearestTransitionCell) + w2*floorDiffPenalty"),
         body("Police/Robber — floor change")],
        [code("REACH_VAULT"),
         body("w1*manhattan(n, VaultPos) + w2*policeDanger(n) + w3*alertZoneBonus(n)"),
         body("Robber — steal phase")],
        [code("ESCAPE_TO_EXIT"),
         body("w1*manhattan(n, ExitPos) + w2*policeDanger(n) + w3*transitionBonus(n)"),
         body("Robber — escape phase")],
        [code("REACH_ALERT_ZONE"),
         body("w1*manhattan(n, nearestAlertZone) + w2*policeDanger(n)"),
         body("Robber — distract")],
    ], colWidths=[(W-2*MARGIN)*0.22,(W-2*MARGIN)*0.52,(W-2*MARGIN)*0.26],
    style=TableStyle([
        ('BACKGROUND',(0,0),(-1,0),C_DARK),('TEXTCOLOR',(0,0),(-1,0),C_WHITE),
        ('GRID',(0,0),(-1,-1),0.4,C_BORDER),
        ('ROWBACKGROUNDS',(0,1),(-1,-1),[C_PANEL,C_WHITE]),
        ('TOPPADDING',(0,0),(-1,-1),5),('BOTTOMPADDING',(0,0),(-1,-1),5),
        ('LEFTPADDING',(0,0),(-1,-1),6),('VALIGN',(0,0),(-1,-1),'TOP'),
        ('FONTSIZE',(0,1),(-1,-1),8.5),
    ])),
    sp(6),
    h2("9.3  Weight Scaling by Difficulty"),
    Table([
        [Paragraph("<b>Difficulty</b>", S("th",fontSize=9,fontName="Helvetica-Bold")),
         Paragraph("<b>w1 (distance)</b>", S("th",fontSize=9,fontName="Helvetica-Bold")),
         Paragraph("<b>w2 (danger/proximity)</b>", S("th",fontSize=9,fontName="Helvetica-Bold")),
         Paragraph("<b>Behaviour</b>", S("th",fontSize=9,fontName="Helvetica-Bold"))],
        [body("Easy"),   body("1.0"), body("0.3"), body("Police less aggressive, forgives mistakes")],
        [body("Normal"), body("1.0"), body("0.7"), body("Balanced pursuit and protection")],
        [body("Hard"),   body("1.0"), body("1.5"), body("Police very aggressive, blocks paths")],
    ], colWidths=[(W-2*MARGIN)*0.18,(W-2*MARGIN)*0.18,(W-2*MARGIN)*0.24,(W-2*MARGIN)*0.40],
    style=TableStyle([
        ('BACKGROUND',(0,0),(-1,0),C_DARK),('TEXTCOLOR',(0,0),(-1,0),C_WHITE),
        ('GRID',(0,0),(-1,-1),0.4,C_BORDER),
        ('ROWBACKGROUNDS',(0,1),(-1,-1),[C_PANEL,C_WHITE]),
        ('TOPPADDING',(0,0),(-1,-1),5),('BOTTOMPADDING',(0,0),(-1,-1),5),
        ('LEFTPADDING',(0,0),(-1,-1),6),
    ])),
    PageBreak(),
]

# ── 10. DEBUG WINDOW ──────────────────────────────────────────────────────────
story += [
    h1("10. DEBUG WINDOW — SPECIFICATION"),
    sp(),
    h2("10.1  One Window Per Police Agent"),
    body("At game start, GameEngineGUI spawns one debug_window process per Police agent:"),
    code('// For N police agents:'),
    code('for (int i = 0; i < policeList.size(); i++) {'),
    code('    string snapshotFile = "debug_snapshot_police" + to_string(i+1) + ".txt";'),
    code('    pid_t pid = fork();'),
    code('    if (pid == 0) {'),
    code('        string title = "Police" + to_string(i+1) + " Debug";'),
    code('        execl("./debug_window", "./debug_window",'),
    code('              snapshotFile.c_str(), title.c_str(), nullptr);'),
    code('        _exit(1);'),
    code('    }'),
    code('}'),
    sp(6),
    body("Each Police agent's PoliceAIPlanner writes to its own snapshot file after every turn. "
         "The debug window reads its assigned file continuously (30fps refresh)."),
    sp(6),
    h2("10.2  Window Layout — Three Panels"),
    body("Window minimum size: 1100 × 750. Font: monospace (Courier/Menlo). "
         "Three equal-width panels side by side, full height."),
    sp(4),
    Table([
        [Paragraph("<b>Panel 1 — Goal Stack</b>", S("th",fontSize=9,fontName="Helvetica-Bold",textColor=C_WHITE)),
         Paragraph("<b>Panel 2 — Active Operator</b>", S("th",fontSize=9,fontName="Helvetica-Bold",textColor=C_WHITE)),
         Paragraph("<b>Panel 3 — A* Successors</b>", S("th",fontSize=9,fontName="Helvetica-Bold",textColor=C_WHITE))],
        [body("Header: GOAL STACK\nAgent: PoliceN\nGoalType: PROTECT_VAULT\n─────────────\n"
              "[ACTIVE]  MOVE(Police1, Vault)  ← yellow\n[PENDING] sameFloor ^ path...\n"
              "[PENDING] protect(Police1, Vault)\n─────────────\nCOMPLETED\n~~stayHalt(Police1)~~\n"
              "CANCELLED\nrespond(Police1, Alert)"),
          body("Header: ACTIVE OPERATOR\nCurrent Cell: [x,y,z]\nTarget Cell:  [x,y,z]\n─────────────\n"
               "Operator: MOVE\n─────────────\nPreconditions:\n  ✓ sameFloor(Police1, Vault)\n"
               "  ✓ pathAvailable(Police1, Vault)\n\nEffects:\n  → Police1.pos = Vault.pos"),
          body("Header: A* SUCCESSORS\nGoalType: PROTECT_VAULT\n─────────────\n"
               "Pos         g    h    f    *\n─────────────\n[7,6,0]  1.0  4.2  5.2\n"
               "[7,7,0]  1.0  3.0  4.0  ★  ← yellow row\n[8,7,0]  1.0  3.8  4.8\n"
               "─────────────\nChosen Node\nPos:[7,7,0] g=1.0 h=3.0 f=4.0")],
    ], colWidths=[(W-2*MARGIN)/3]*3,
    style=TableStyle([
        ('BACKGROUND',(0,0),(-1,0),C_DARK),('TEXTCOLOR',(0,0),(-1,0),C_WHITE),
        ('GRID',(0,0),(-1,-1),0.4,C_BORDER),
        ('BACKGROUND',(0,1),(-1,1),C_PANEL),
        ('TOPPADDING',(0,0),(-1,-1),6),('BOTTOMPADDING',(0,0),(-1,-1),6),
        ('LEFTPADDING',(0,0),(-1,-1),6),('VALIGN',(0,0),(-1,-1),'TOP'),
        ('FONTSIZE',(0,1),(-1,-1),8),
    ])),
    sp(8),
    h2("10.3  Debug Window Title"),
    body("Each window title must include the agent name: 'Police1 — Debug Window', 'Police2 — Debug Window', etc. "
         "This is set via the second execl argument and used in the Raylib window title."),
    PageBreak(),
]

# ── 11. SNAPSHOT FILE FORMAT ──────────────────────────────────────────────────
story += [
    h1("11. SNAPSHOT FILE FORMAT"),
    sp(),
    body("Each Police agent writes to its own snapshot file. Format is JSON-like structured text. "
         "Written atomically (write to .tmp then rename to avoid partial reads)."),
    sp(4),
    code('{'),
    code('  "agent": "Police1",'),
    code('  "state": "PROTECT_VAULT",'),
    code('  "currentCell": [7, 6, 0],'),
    code('  "targetCell": [7, 7, 0],'),
    code('  "goalStack": ['),
    code('    {'),
    code('      "expression": "MOVE(Police1, Vault)",'),
    code('      "isOperator": true,'),
    code('      "operatorName": "MOVE",'),
    code('      "status": "ACTIVE",'),
    code('      "preconditions": ["sameFloor(Police1, Vault)", "pathAvailable(Police1, Vault)"],'),
    code('      "effects": ["Police1.pos=Vault.pos"]'),
    code('    },'),
    code('    {'),
    code('      "expression": "sameFloor(Police1, Vault) ^ pathAvailable(Police1, Vault)",'),
    code('      "isOperator": false,'),
    code('      "status": "PENDING",'),
    code('      "preconditions": [],  "effects": []'),
    code('    },'),
    code('    { "expression": "protect(Police1, Vault)", "isOperator": false, "status": "PENDING", ... }'),
    code('  ],'),
    code('  "completedGoals": ['),
    code('    { "expression": "stayHalt(Police1)", "status": "COMPLETED", ... }'),
    code('  ],'),
    code('  "cancelledGoals": [],'),
    code('  "astarSuccessors": ['),
    code('    { "label": "[7,6,0]", "pos": [7,6,0], "g": 1, "h": 4.2, "f": 5.2, "chosen": false },'),
    code('    { "label": "[7,7,0]", "pos": [7,7,0], "g": 1, "h": 3.0, "f": 4.0, "chosen": true }'),
    code('  ],'),
    code('  "chosenNode": { "label": "[7,7,0]", "pos": [7,7,0], "g": 1, "h": 3.0, "f": 4.0 },'),
    code('  "activeGoalType": "PROTECT_VAULT"'),
    code('}'),
    sp(6),
    note("goalStack array is ordered top-first (index 0 = top of stack). "
         "The debug window must display it top-first. "
         "astarSuccessors contains only cells adjacent to the search start (4-directional filter)."),
    PageBreak(),
]

# ── 12. GAME ENGINE TURN LOOP ─────────────────────────────────────────────────
story += [
    h1("12. GAME ENGINE TURN LOOP (GameEngineGUI)"),
    sp(),
    h2("12.1  Human-Robber Turn Sequence"),
    Table([
        [Paragraph("<b>Step</b>", S("th",fontSize=9,fontName="Helvetica-Bold")),
         Paragraph("<b>Action</b>", S("th",fontSize=9,fontName="Helvetica-Bold"))],
        [body("1"), body("Read WASD input → compute targetPos.")],
        [body("2"), body("Validate move: not wall, not out of bounds, delta==1.")],
        [body("3"), body("setPlayerPosition(targetPos) → auto floor transition if on STAIRS/ELEVATOR.")],
        [body("4"), body("refreshVaultCollectionState(): if Robber.pos == VaultPos → set vaultStolen, call rules.notifyVaultStolen().")],
        [body("5"), body("Check if Robber stepped on ALERT_ZONE: call rules.notifyAlertTriggered(pos), set alertTriggered=true.")],
        [body("6"), body("updateAI(): run every Police agent's planner. Each planner gets updated WorldState.")],
        [body("7"), body("checkWinConditions(): any Police.pos==Robber.pos → POLICE_WON. Robber.pos==Exit+hasVault → ROBBER_WON.")],
        [body("8"), body("Write snapshot for each Police agent (separate files).")],
        [body("9"), body("Render frame.")],
        [body("10"),body("turnCount++.")],
    ], colWidths=[(W-2*MARGIN)*0.06,(W-2*MARGIN)*0.94],
    style=TableStyle([
        ('BACKGROUND',(0,0),(-1,0),C_DARK),('TEXTCOLOR',(0,0),(-1,0),C_WHITE),
        ('GRID',(0,0),(-1,-1),0.4,C_BORDER),
        ('ROWBACKGROUNDS',(0,1),(-1,-1),[C_PANEL,C_WHITE]),
        ('TOPPADDING',(0,0),(-1,-1),5),('BOTTOMPADDING',(0,0),(-1,-1),5),
        ('LEFTPADDING',(0,0),(-1,-1),6),
    ])),
    sp(8),
    h2("12.2  WorldState Fields Required by Planners"),
    code("struct WorldState {"),
    code("    const Grid3D* grid;"),
    code("    Position policePos;          // this police agent's position"),
    code("    Position robberPos;"),
    code("    Position vaultPos;"),
    code("    Position exitPos;"),
    code("    Position alertPos;           // from RuleEngine.getAlertPos()"),
    code("    bool vaultStolen;            // from RuleEngine.isVaultStolen()"),
    code("    bool robberHasVault;"),
    code("    bool alertTriggered;         // from RuleEngine.isAlertActive()"),
    code("    bool boostActive;            // from RuleEngine.isBoostActive()"),
    code("    bool policeWon;"),
    code("    bool robberWon;"),
    code("    vector<Position> policePositions;  // all police positions"),
    code("    GoalType activeGoalType;"),
    code("    Position activeTargetPos;"),
    code("    int activeTargetFloor;"),
    code("};"),
    PageBreak(),
]

# ── 13. MULTI-POLICE ARCHITECTURE ────────────────────────────────────────────
story += [
    h1("13. MULTI-POLICE ARCHITECTURE"),
    sp(),
    h2("13.1  Per-Agent Independence"),
    bullet("Each PoliceAI has its own PoliceAIPlanner instance."),
    bullet("Each PoliceAIPlanner has its own GoalStack, lastVaultStolen, lastAlertTriggered, robberMadeFirstMoveOnFloor."),
    bullet("Each planner writes to its own snapshot file: debug_snapshot_police1.txt, debug_snapshot_police2.txt, etc."),
    bullet("Each planner's runTurn() receives a WorldState with that agent's policePos."),
    bullet("Police agents do not share or read each other's goal stacks."),
    sp(6),
    h2("13.2  Floor-Based Activation Tracking"),
    body("Each PoliceAIPlanner must track per-floor first-move detection independently:"),
    code("// In PoliceAIPlanner:"),
    code("bool robberMadeFirstMoveOnFloor[3] = {false, false, false};  // indexed by floor 0-2"),
    code(""),
    code("// In handleInterrupts():"),
    code("int robberFloor = world.robberPos.z;"),
    code("if (!robberMadeFirstMoveOnFloor[robberFloor] && world.robberPos != prevRobberPos) {"),
    code("    robberMadeFirstMoveOnFloor[robberFloor] = true;"),
    code("    if (world.policePos.z == robberFloor) {  // this police is on same floor"),
    code("        // activate this police"),
    code("        if (world.vaultPos.z == robberFloor) {"),
    code('            pushProtectVaultBlock(world);   // protect(PoliceN, Vault)'),
    code("        } else {"),
    code('            pushProtectUpstairsBlock(world); // protect(PoliceN, Upstairs)'),
    code("        }"),
    code("    }"),
    code("}"),
    sp(6),
    h2("13.3  Police Stay On Their Floor"),
    body("A Police agent on Floor 0 must NOT autonomously switch to Floor 1 when Robber moves there, "
         "unless vaultStolen OR alertBoostActive. The protect(Police1, Upstairs) goal moves police "
         "toward the transition cell, but the planner should NOT push further floor goals until "
         "either condition is met."),
    note("Exception: If Police1 is already mid-path to Upstairs via protect(Police1, Upstairs) "
         "and vault is stolen, the interrupt fires, cancels that goal, and pushes catch(Police1, Robber) "
         "which now includes the floor transition logic."),
    sp(6),
    h2("13.4  Alert Response — Floor Restriction"),
    body("respond(PoliceN, Alert) is only pushed for Police agents on the SAME FLOOR as the alert zone. "
         "Police on other floors ignore alert triggers unless vault is stolen (which triggers catch instead)."),
    PageBreak(),
]

# ── 14. RULE ENGINE REQUIREMENTS ─────────────────────────────────────────────
story += [
    h1("14. RULE ENGINE REQUIREMENTS"),
    sp(),
    body("RuleEngine must expose these methods cleanly for planners to use:"),
    sp(4),
    Table([
        [Paragraph("<b>Method</b>", S("th",fontSize=9,fontName="Helvetica-Bold")),
         Paragraph("<b>Returns</b>", S("th",fontSize=9,fontName="Helvetica-Bold")),
         Paragraph("<b>Description</b>", S("th",fontSize=9,fontName="Helvetica-Bold"))],
        [code("isVaultStolen()"),    body("bool"),     body("True after Robber collected vault.")],
        [code("isAlertActive()"),    body("bool"),     body("True while boost turns remaining > 0.")],
        [code("isBoostActive()"),    body("bool"),     body("Same as isAlertActive(). Police can use stairs.")],
        [code("getAlertPos()"),      body("Position"), body("Last triggered alert zone position.")],
        [code("getAlertFloor()"),    body("int"),      body("Floor of last alert zone.")],
        [code("notifyVaultStolen()"),body("void"),     body("Called by GameEngineGUI when vault collected.")],
        [code("notifyAlertTriggered(pos)"),body("void"),body("Called when Robber steps on ALERT_ZONE.")],
        [code("getPredictionDepth()"),body("int"),     body("Depth for PredictionEngine (difficulty-scaled).")],
        [code("getWeights(GoalType)"),body("HeuristicWeights"), body("Difficulty-tuned weights for each goal type.")],
    ], colWidths=[(W-2*MARGIN)*0.32,(W-2*MARGIN)*0.16,(W-2*MARGIN)*0.52],
    style=TableStyle([
        ('BACKGROUND',(0,0),(-1,0),C_DARK),('TEXTCOLOR',(0,0),(-1,0),C_WHITE),
        ('GRID',(0,0),(-1,-1),0.4,C_BORDER),
        ('ROWBACKGROUNDS',(0,1),(-1,-1),[C_PANEL,C_WHITE]),
        ('TOPPADDING',(0,0),(-1,-1),5),('BOTTOMPADDING',(0,0),(-1,-1),5),
        ('LEFTPADDING',(0,0),(-1,-1),6),('VALIGN',(0,0),(-1,-1),'TOP'),
        ('FONTSIZE',(0,1),(-1,-1),8.5),
    ])),
    sp(8),
    h2("14.1  HeuristicWeights Struct"),
    code("struct HeuristicWeights {"),
    code("    float w1;  // distance-to-goal weight"),
    code("    float w2;  // danger/proximity weight"),
    code("    float w3;  // bonus/penalty weight (CCTV, alert, transition)"),
    code("    float w4;  // secondary bonus weight"),
    code("};"),
    PageBreak(),
]

# ── 15. WHAT NOT TO CHANGE ────────────────────────────────────────────────────
story += [
    h1("15. DO NOT CHANGE — PROTECTED COMPONENTS"),
    sp(),
    body("These components must NOT be modified when implementing the AI changes. "
         "They are stable and correct."),
    sp(6),
    Table([
        [Paragraph("<b>File / Component</b>", S("th",fontSize=9,fontName="Helvetica-Bold")),
         Paragraph("<b>Why Protected</b>", S("th",fontSize=9,fontName="Helvetica-Bold"))],
        [body("Grid3D.h / Grid3D.cpp"),          body("Cell types, floor layout, map loading, getVaultPos/getExitPos — all correct.")],
        [body("RaylibRenderer.cpp"),              body("Visual rendering style, tile colors, agent markers — keep as-is.")],
        [body("Win/loss condition checks"),       body("In GameEngineGUI.cpp checkWinConditions() — conditions are correct.")],
        [body("debug_main.cpp structure"),        body("Separate process architecture is correct. Only update snapshot schema reading.")],
        [body("RuleEngine alert/boost tracking"), body("Internal boost counter logic is correct. Only add new public methods.")],
        [body("AStar3D findPath core"),           body("Core A* algorithm is correct. Only add GoalType parameter passing.")],
    ], colWidths=[(W-2*MARGIN)*0.35,(W-2*MARGIN)*0.65],
    style=TableStyle([
        ('BACKGROUND',(0,0),(-1,0),C_DARK),('TEXTCOLOR',(0,0),(-1,0),C_WHITE),
        ('GRID',(0,0),(-1,-1),0.4,C_BORDER),
        ('ROWBACKGROUNDS',(0,1),(-1,-1),[colors.HexColor("#fdecea"),C_PANEL]),
        ('TOPPADDING',(0,0),(-1,-1),5),('BOTTOMPADDING',(0,0),(-1,-1),5),
        ('LEFTPADDING',(0,0),(-1,-1),6),
    ])),
    sp(8),
    h2("15.1  Implementation Order"),
    body("When handing this document to an AI coding assistant, instruct it to implement in this order:"),
    Table([
        [Paragraph("<b>Order</b>", S("th",fontSize=9,fontName="Helvetica-Bold")),
         Paragraph("<b>File</b>", S("th",fontSize=9,fontName="Helvetica-Bold")),
         Paragraph("<b>Changes</b>", S("th",fontSize=9,fontName="Helvetica-Bold"))],
        [body("1"), code("RuleEngine.h/.cpp"),          body("Add getAlertFloor(), getWeights(), HeuristicWeights struct.")],
        [body("2"), code("GoalStack.h/.cpp"),            body("Add cancelAll(), fix getStack() top-first ordering, fix promoteTopToActive().")],
        [body("3"), code("HeuristicEngine.h/.cpp"),      body("Replace monolithic heuristic with compute(node, GoalType, WorldState).")],
        [body("4"), code("AStar3D.cpp"),                 body("Add GoalType parameter, pass to HeuristicEngine::compute().")],
        [body("5"), code("PoliceAIPlanner.h/.cpp"),      body("Full redesign per Section 6 and 8. Per-floor activation. Interrupt system.")],
        [body("6"), code("RobberAIPlanner.h/.cpp"),      body("Redesign goal stack per Section 5.2.")],
        [body("7"), code("PoliceAI.cpp"),                body("Thin wrapper. Delegates all to PoliceAIPlanner.")],
        [body("8"), code("RobberAI.cpp"),                body("Thin wrapper. Delegates all to RobberAIPlanner.")],
        [body("9"), code("GameEngineGUI.cpp"),           body("Multi-snapshot writer (one per police). Multi-window spawner. Turn loop update.")],
        [body("10"),code("DebugWindow.cpp/.h"),          body("Three-panel layout. Title includes agent name. Accept agent title arg.")],
    ], colWidths=[(W-2*MARGIN)*0.06,(W-2*MARGIN)*0.30,(W-2*MARGIN)*0.64],
    style=TableStyle([
        ('BACKGROUND',(0,0),(-1,0),C_DARK),('TEXTCOLOR',(0,0),(-1,0),C_WHITE),
        ('GRID',(0,0),(-1,-1),0.4,C_BORDER),
        ('ROWBACKGROUNDS',(0,1),(-1,-1),[C_PANEL,C_WHITE]),
        ('TOPPADDING',(0,0),(-1,-1),5),('BOTTOMPADDING',(0,0),(-1,-1),5),
        ('LEFTPADDING',(0,0),(-1,-1),6),
    ])),
    PageBreak(),
]

# ── 16. QUICK REFERENCE CARD ─────────────────────────────────────────────────
story += [
    h1("16. QUICK REFERENCE — POLICE GOAL PUSH ORDER"),
    sp(),
    body("This section shows the EXACT push sequence for every scenario. "
         "Remember: the LAST item pushed becomes the TOP of the stack."),
    sp(6),
    h2("protect(PoliceN, Vault) Block — push in this order:"),
    code("goalStack.push( makeGoal('protect(PoliceN, Vault)', {...}, {...}) );              // 1st pushed → bottom"),
    code("goalStack.push( makeGoal('sameFloor(PoliceN, Vault) ^ pathAvailable(...)', {},{}) ); // 2nd"),
    code("// IF different floor:"),
    code("goalStack.push( makeOp('MOVE(PoliceN, Upstairs)', 'MOVE', {...}, {...}) );       // 3rd if needed"),
    code("goalStack.push( makeOp('MOVE(PoliceN, Vault)', 'MOVE', {...}, {...}) );          // LAST → top"),
    sp(8),
    h2("respond(PoliceN, Alert) Block — push in this order:"),
    code("goalStack.push( makeGoal('respond(PoliceN, Alert)', {...}, {...}) );              // 1st pushed → bottom of block"),
    code("goalStack.push( makeGoal('sameFloor(PoliceN, Alert) ^ pathAvailable(...)', {},{}) ); // 2nd"),
    code("// IF different floor:"),
    code("goalStack.push( makeOp('MOVE(PoliceN, Upstairs)', 'MOVE', {...}, {...}) );       // 3rd if needed"),
    code("goalStack.push( makeOp('MOVE(PoliceN, Alert)', 'MOVE', {...}, {...}) );          // LAST → top"),
    sp(8),
    h2("catch(PoliceN, Robber) Block — push in this order:"),
    code("goalStack.push( makeGoal('catch(PoliceN, Robber)', {...}, {...}) );              // 1st pushed → bottom"),
    code("goalStack.push( makeGoal('sameFloor(PoliceN, Robber) ^ pathAvailable(...)', {},{}) ); // 2nd"),
    code("// IF different floor:"),
    code("goalStack.push( makeOp('onTransitionCell(PoliceN)', ...) );                      // 3rd if needed"),
    code("goalStack.push( makeOp('MOVE(PoliceN, Upstairs/Downstairs)', 'MOVE', {...}, {...}) ); // 4th"),
    code("goalStack.push( makeOp('MOVE(PoliceN, Robber)', 'MOVE', {...}, {...}) );         // LAST → top"),
    sp(8),
    note("The LAST push is ALWAYS the immediate executable action (a MOVE operator). "
         "This ensures executeTop() always finds an operator to execute first, "
         "then works down as goals complete."),
    sp(8),
    h2("Alert → Protect Resume Rule"),
    body("When respond(PoliceN, Alert) is COMPLETED (police reached alertPos):"),
    code("goalStack.markComplete();          // completes respond"),
    code("goalStack.finalizeCompleted();     // pops respond block"),
    code("// Now protect block below is exposed — push it again fresh:"),
    code("pushProtectVaultBlock(world);      // re-push protect(PoliceN, Vault)"),
    sp(4),
    note("Do NOT assume the old protect block survived below respond. Always re-push protect after alert is resolved."),
    PageBreak(),
]

# ── 17. COMMON MISTAKES TO AVOID ─────────────────────────────────────────────
story += [
    h1("17. COMMON MISTAKES TO AVOID"),
    sp(),
    Table([
        [Paragraph("<b>Mistake</b>", S("th",fontSize=9,fontName="Helvetica-Bold")),
         Paragraph("<b>Consequence</b>", S("th",fontSize=9,fontName="Helvetica-Bold")),
         Paragraph("<b>Correct Behaviour</b>", S("th",fontSize=9,fontName="Helvetica-Bold"))],
        [body("Push MOVE before pushing the goal it serves"),
         body("Stack is inverted — goal is on top, operator never executes"),
         body("Push goal FIRST (goes to bottom), push operator LAST (becomes top).")],
        [body("Checking goalStack.getStack()[0] as bottom"),
         body("Display and logic are inverted"),
         body("getStack() returns top-first. Index 0 = top. back() of internal vector = top.")],
        [body("Using same snapshot file for all police"),
         body("Debug windows show same agent, multi-agent invisible"),
         body("One snapshot file per police: debug_snapshot_police1.txt etc.")],
        [body("Pushing catch without cancelAll() first"),
         body("Old protect/respond goals pile up below catch"),
         body("Always cancelAll() before pushCatchBlock() on vault stolen.")],
        [body("Not re-planning MOVE(Police, Robber) each turn"),
         body("Police chases old robber position, never catches moving target"),
         body("MOVE(Police, Robber) calls A* fresh with current world.robberPos every turn it is ACTIVE.")],
        [body("Police crossing floors without vault stolen"),
         body("Breaks game rule, police cheats"),
         body("Only push MOVE(Police, Upstairs) if vaultStolen OR alertBoostActive.")],
        [body("Alert response for police on wrong floor"),
         body("Police across floors chases alert it cannot reach"),
         body("Only push respond block for police whose policePos.z == alertPos.z.")],
        [body("Empty goalExpression strings"),
         body("Debug window shows blank boxes"),
         body("EVERY GoalEntry push must set goalExpression to a non-empty string from Section 5.")],
        [body("Not writing A* successor log to snapshot"),
         body("A* panel shows 'No search data'"),
         body("After every A* call: lastAstarTrace = AStar3D::getLastSearchTrace(); write to snapshot.")],
    ], colWidths=[(W-2*MARGIN)*0.30,(W-2*MARGIN)*0.32,(W-2*MARGIN)*0.38],
    style=TableStyle([
        ('BACKGROUND',(0,0),(-1,0),C_DARK),('TEXTCOLOR',(0,0),(-1,0),C_WHITE),
        ('GRID',(0,0),(-1,-1),0.4,C_BORDER),
        ('ROWBACKGROUNDS',(0,1),(-1,-1),[colors.HexColor("#fdecea"),C_PANEL]),
        ('TOPPADDING',(0,0),(-1,-1),5),('BOTTOMPADDING',(0,0),(-1,-1),5),
        ('LEFTPADDING',(0,0),(-1,-1),6),('VALIGN',(0,0),(-1,-1),'TOP'),
        ('FONTSIZE',(0,1),(-1,-1),8.5),
    ])),
    sp(12),
    HRFlowable(width="100%", thickness=1, color=C_ACCENT, spaceAfter=8),
    Paragraph("End of Rule Book — 3D Heist Simulator AI Implementation Guide v1.0",
              S("footer", fontSize=9, textColor=C_GREY, alignment=TA_CENTER, fontName="Helvetica-Oblique")),
]

# ── BUILD PDF ─────────────────────────────────────────────────────────────────
doc = SimpleDocTemplate(
    "RULEBOOK.pdf",
    pagesize=A4,
    leftMargin=MARGIN, rightMargin=MARGIN,
    topMargin=MARGIN, bottomMargin=MARGIN,
    title="3D Heist Simulator — AI Rule Book",
    author="Project Team",
)
doc.build(story)
print("PDF generated successfully.")