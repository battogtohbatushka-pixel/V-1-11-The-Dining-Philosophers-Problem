from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib import colors
from reportlab.lib.units import cm
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle, PageBreak, Preformatted
d='/usr/share/fonts/truetype/dejavu/'
pdfmetrics.registerFont(TTFont('F',d+'DejaVuSans.ttf'));pdfmetrics.registerFont(TTFont('FB',d+'DejaVuSans-Bold.ttf'));pdfmetrics.registerFont(TTFont('M',d+'DejaVuSansMono.ttf'))
B=ParagraphStyle('b',fontName='F',fontSize=10,leading=14,spaceAfter=5)
H1=ParagraphStyle('h1',parent=B,fontName='FB',fontSize=14,leading=18,spaceBefore=10,spaceAfter=6)
T=ParagraphStyle('t',parent=B,fontName='FB',fontSize=20,leading=26,spaceAfter=10)
C=ParagraphStyle('c',parent=B,fontSize=8.5,leading=11)
CODE=ParagraphStyle('code',fontName='M',fontSize=8,leading=10)
def p(x):return Paragraph(x,B)
def h(x):return Paragraph(x,H1)
def code(x):return Preformatted(x,CODE)
def tbl(rows,w=None):
    t=Table([[Paragraph(str(c),C) for c in r] for r in rows],colWidths=w,repeatRows=1)
    t.setStyle(TableStyle([('GRID',(0,0),(-1,-1),.4,colors.grey),('BACKGROUND',(0,0),(-1,0),colors.HexColor('#e6e9ef')),('VALIGN',(0,0),(-1,-1),'TOP')]));return t
def bul(items):return [Paragraph('• '+i,B) for i in items]
def build(name,story):
    SimpleDocTemplate(name,pagesize=A4,leftMargin=2*cm,rightMargin=2*cm,topMargin=2*cm,bottomMargin=2*cm,title=name).build(story)
tests=open('tests/results.txt').read().splitlines()
res=[l[6:] for l in tests if l.startswith('PASS') and 'stress' not in l]
scal=[l.split() for l in open('examples/scaling.txt')]
srows=[['N','Нийт хоол','Зөрчил','Нэгэн зэрэг идсэн max','Max хүлээлт (мс)','Нийт хугацаа (мс)']]
for l in scal:
    kv=dict(x.split('=') for x in l[1:]);srows.append([kv['n'],kv['total'],kv['violations'],kv['max_concurrent_eating'],kv['max_wait_ms'],kv['elapsed_ms']])
# ---------------- REPORT
s=[Paragraph('F.CSM302 Үйлдлийн систем — Бие даалт 1',B),Paragraph('The Dining-Philosophers Problem',T),
 p('Хуваарилалтын код: <b>[КОД]</b><br/>Гишүүд: <b>[ГИШҮҮД]</b><br/>Сурах бичиг: Silberschatz, Operating System Concepts, Ch.7 Project 11'),
 h('Хураангуй'),
 p('Dining-Philosophers бодлогыг monitor (нэг mutex + philosopher бүрт нэг condition variable) ашиглан Pthreads-ээр C хэл дээр хэрэгжүүлэв. Deadlock болон starvation-ийг ойлгоход зориулж thinking, hungry, eating төлөв ба fork эзэмшилтийг харуулдаг интерактив HTML UI бүтээв. UI нь Monitor болон Naive (зүүн→баруун) гэсэн хоёр алгоритмыг харьцуулж, Naive горимд deadlock-ийг илрүүлнэ. Автомат 45 тест (үндсэн, захын, стресс, алдааны) бүгд амжилттай.'),
 h('1. Удиртгал'),p('Зорилго: нийтлэг нөөц (fork) -ийн синхрончлол, deadlock-аас сэргийлэх аргыг хэрэгжүүлж, туршиж, дүрслэх. Хамрах хүрээ: 2–64 philosopher, тохируулах боломжтой thinking/eating хугацаа, seed.'),
 h('2. Сурах бичгийн шаардлага'),
 *bul(['Philosopher бүр thread, thinking ба eating үе ээлжилнэ (санамсаргүй хугацаа).','Хөрш philosopher нар нэгэн зэрэг идэж болохгүй.','Monitor-ийн pickup_forks(i) ба return_forks(i) үйлдэл, state[] (THINKING, HUNGRY, EATING), self[] condition variable.','Pthreads mutex/condition variable ашиглана.','Тоо ба хугацааны параметрийг өөрчилж болно, thinking/hungry/eating төлөв ба fork эзэмшилтийг дүрсэлнэ (багшийн UI шаардлага).']),
 h('3. Онолын үндэс'),
 p('<b>Deadlock</b> дөрвөн нөхцөлтэй: mutual exclusion, hold and wait, no preemption, circular wait. Naive шийдэлд philosopher бүр эхлээд зүүн fork-оо авч, дараа нь баруун fork-оо хүлээдэг тул бүгд зүүн fork-оо барьсан үед circular wait үүсч deadlock болно. <b>Monitor шийдэл</b> нь fork-ийг "хоёулангий нь нэг дор авах" атом үйлдэл болгодог (hold and wait-ийг устгана): philosopher зөвхөн хоёр хөрш нь идэхгүй байгаа үед EATING төлөвт орно. Starvation-ийг тус шийдэл бүрэн үгүйсгэдэггүй (хөрш нар ээлжлэн идвэл тухайн philosopher удаан хүлээж болзошгүй).'),
 h('4. Системийн зохиомж'),
 code('''philosopher thread i:  loop { think(); pickup(i); eat(); putdown(i) }

monitor (mutex mon):
  pickup(i):   state[i]=HUNGRY; test(i); while(state[i]!=EATING) wait(self[i])
  putdown(i):  state[i]=THINKING; test(left); test(right)
  test(i):     if state[i]==HUNGRY && state[left]!=EATING && state[right]!=EATING
                   state[i]=EATING; signal(self[i])'''),
 p('Fork i нь philosopher i-ийн зүүн, philosopher (i+1) mod N-ийн баруун талд оршино. Системд хоёр нэгж бий: <i>src/philosophers.c</i> (бодит Pthreads) ба <i>src/dining_philosophers.html</i> (UI симуляц). Симуляц нь ижил state/test логикийг дискрет цагийн алхмаар ажиллуулна.'),
 h('5. Хэрэгжилт'),
 *bul(['Хэл: C (C99, gcc, -pthread); UI: HTML + JavaScript + SVG, гадаад сангүй.','Нэг mutex + N condition variable, rand_r-ээр philosopher бүрийн тусдаа seed (давтагдах боломжтой).','Инвариант шалгалт: pickup дотор хөрш нь идэж байвал violations++ ; нэгэн зэрэг идэж буй хамгийн их тоог (max_concurrent_eating) тоолно.','Оролтыг шалгана: -n 2..64, -m≥1, -t/-e нь 0&lt;min≤max; алдаа гарвал мессеж хэвлэж rc=2.','UI: Start/Pause/Step/Reset, N, thinking/eating min/max, seed, хурд, алгоритм сонголт; буруу оролтод улаан мэдээлэл.']),
 h('6. Туршилт ба UI дүрслэл'),
 p('UI дээр өөрчилж болох зүйл: philosopher-ийн тоо (2–12), thinking/eating хугацаа, seed, хурд, алгоритм. Ажиглах зүйл: philosopher-ийн өнгө (Thinking = цэнхэр, Hungry = шар, Eating = ногоон), fork-ийн байрлал (хоосон үед ширээн дээр саарал, эзэмшсэн үед эзэмшигчийн хажууд эзэмшигчийн төлөвийн өнгөөр), хүснэгтийн хоол/хүлээлтийн тоо, event log. Naive горимд бүгд hungry болж зүүн fork-оо барьсан үед DEADLOCK анхааруулга гарна.'),
 p('<b>[Screenshot 1: Monitor, N=5]</b> &nbsp; <b>[Screenshot 2: Naive горимын deadlock]</b> &nbsp; <b>[Screenshot 3: N=8, өөр хугацаа]</b> — <i>Эдгээрийг UI-аас өөрсдөө авч энд оруулна уу.</i>'),
 h('7. Туршилтын арга'),
 p('Орчин: Ubuntu 24 (Linux x86-64), gcc, GNU make. Хүлээгдсэн үр дүн: бүх philosopher MEALS удаа идэж дуусна (total = N×MEALS), violations = 0, max_concurrent_eating ≤ ⌊N/2⌋, програм зогсохгүй (deadlock-гүй). Алдаатай оролтод rc=2 ба тодорхой мессеж.'),
 h('8. Үр дүн'),
 p('Дараах хүснэгт нь <i>./philosophers -q -n N -m 5 -t 20,60 -e 20,60 -s 7</i> командын бодит гаралт:'),
 tbl(srows),Spacer(1,6),
 p('Автомат тестийн дүн (tests/results.txt): нийт 45 тест — T1–T7 үндсэн, T8 (30 удаа, N=7, seed 100–129) стресс, E1–E8 алдаа/захын оролт. Бүгд PASS.'),
 tbl([['Тест','Оролт','Хүлээгдсэн','Дүн'],
  ['T1','N=5 m=3 t=20,60 e=20,40','total=15, violations=0','PASS'],['T2','N=2 m=5','total=10, нэг дор ≤1 идэнэ','PASS'],
  ['T4','N=12','deadlock-гүй дуусна','PASS'],['T5','N=64 (дээд хязгаар)','дуусна','PASS'],
  ['T6','min=max (тогтмол хугацаа)','дуусна','PASS'],['E1/E2','N=1, N=65','rc=2 + мессеж','PASS'],
  ['E5','-t 300,100','rc=2 + мессеж','PASS'],['E4','-m 0','rc=2','PASS']],[1.5*cm,5.5*cm,6*cm,2*cm]),
 h('9. Хэлэлцүүлэг'),
 p('N нэмэгдэхэд нэгэн зэрэг идэх philosopher-ийн дээд тоо ⌊N/2⌋-д ойртдог (N=12 үед 6) нь параллелизмын онолын дээд хязгаартай нийцнэ. Хүлээлтийн хугацаа нь санамсаргүй хугацаанаас хамаарна. Хязгаарлалт: (1) monitor шийдэл starvation-аас албан ёсоор хамгаалдаггүй; (2) UI нь бодит Pthreads биш дискрет симуляц, цаг хугацаа нь simulated; (3) C програм нь Naive горимыг хэрэгжүүлээгүй (deadlock-ийг зөвхөн UI-д харуулна); (4) UI-г автомат тестлээгүй, гараар шалгана; (5) хүснэгтэд нэг л ажиллалтын үр дүн, статистик дундаж биш.'),
 h('10. Дүгнэлт'),p('Сурах бичгийн monitor шийдлийг Pthreads-ээр хэрэгжүүлж, 45 тестээр зөв ажиллагааг баталгаажуулав. UI нь төлөв ба fork эзэмшлийг бодит логиктой нэгэн адил харуулж, deadlock-ийн механизмыг ойлгоход тусална.'),
 h('GitHub дээрх ажиллагаа'),p('[Гишүүн бүрийн commit, branch, issue-г энд бичнэ]'),
 h('AI ба бэлэн код'),p('Код ба тайлангийн эхний хувилбарыг AI (Claude)-ийн тусламжтай бэлтгэсэн. Алгоритм нь сурах бичгийн monitor шийдэл. Гишүүд шалгасан эсэхээ энд баталгаажуулна.'),
 h('Ашигласан материал'),p('Silberschatz, Galvin, Gagne. Operating System Concepts, Ch.7 (Synchronization Examples; Project 11). POSIX Threads (pthread_mutex, pthread_cond).')]
build('docs/Report.pdf',s)
# ---------------- MANUAL
m=[Paragraph('Dining-Philosophers — Manual',T),
 h('8.1 Хэрэглэгчийн заавар'),
 p('<b>Зорилго:</b> philosopher-ууд thinking → hungry → eating төлвөөр шилжих, fork эзэмших үйл явцыг харуулж, monitor болон naive алгоритмыг харьцуулах.'),
 p('<b>Дэмжих орчин:</b> UI — Chrome/Edge/Firefox/Safari сүүлийн хувилбар. CLI — Linux/macOS (gcc, make, pthread).'),
 p('<b>Нээх:</b> src/dining_philosophers.html файлыг хөтөч дээр хоёр товшиж нээнэ (суулгалт, интернет шаардахгүй).'),
 tbl([['Оролт','Утга','Тайлбар'],['Philosopher-ийн тоо','2–12','Бүхэл тоо'],['Thinking min/max','&gt;0 мс, min≤max','Санамсаргүй хугацааны муж'],['Eating min/max','&gt;0 мс, min≤max',''],['Seed','тоо','Ижил seed = ижил үр дүн'],['Хурд','0.5–10×','Симуляцийн хурд'],['Алгоритм','Monitor / Naive','Naive = deadlock харуулна']],[4*cm,4*cm,8*cm]),Spacer(1,6),
 p('<b>Ажиллуулах, зогсоох, reset:</b> Start/Apply — оруулсан тохиргоогоор шинээр эхлүүлнэ; Pause/Resume — түр зогсооно; Step 100мс — 100 мс-ээр алхамчилна; Reset — бүгдийг цэвэрлэнэ.'),
 p('<b>Дүрслэл:</b> Цэнхэр дугуй = Thinking; Шар = Hungry (fork хүлээж байна); Ногоон = Eating. Саарал хөвөгч шугам = чөлөөтэй fork; өнгөт шугам нь philosopher-ийн хажууд = тэр fork-ийг эзэмшсэн. Улаан анхааруулга = DEADLOCK.'),
 p('<b>Жишээ 1 (Monitor):</b> N=5, thinking 1000–3000, eating 1000–2000, seed 42, Monitor → Start. Хөрш philosopher нэгэн зэрэг ногоон болохгүй, хүснэгтэд хоол нэмэгдэнэ.'),
 p('<b>Жишээ 2 (Deadlock):</b> Алгоритм = Naive, N=5 → Apply. Бүгд hungry болж зүүн fork-оо авмагц DEADLOCK анхааруулга гарч симуляци зогсоно. Seed/хугацаа өөрчилж дахин оролдоно.'),
 p('<b>Түгээмэл алдаа:</b>'),
 tbl([['Мессеж','Шалтгаан','Шийдэл'],['Philosopher-ийн тоо 2…12','N хязгаараас хэтэрсэн','2–12 бүхэл тоо оруул'],['min нь max-аас их байж болохгүй','Муж буруу','min ≤ max болго'],['Хугацаа 0-ээс их байх ёстой','0 эсвэл сөрөг','Эерэг тоо оруул']],[5*cm,5*cm,6*cm]),
 PageBreak(),h('8.2 Operational заавар'),
 code('''git clone <REPOSITORY-URL> dining-philosophers
cd dining-philosophers'''),
 p('<b>Орчин:</b> Ubuntu 24 x86-64 (мөн Linux/macOS), gcc ≥ 9, GNU make, pthread. Нэмэлт dependency байхгүй. UI-д хөтөч л хэрэгтэй.'),
 p('<b>Build:</b>'),code('make'),p('<b>Run:</b>'),code('./philosophers -n 5 -m 3 -t 100,300 -e 100,200 -s 42'),
 p('Параметр: -n philosopher (2..64), -m хоолны тоо (≥1), -t thinking min,max мс, -e eating min,max мс, -s seed, -q log-гүй.'),
 p('<b>Үндсэн тест:</b>'),code('make test      # эсвэл bash tests/run_tests.sh'),
 p('Хүлээгдсэн: сүүлийн мөр <b>ALL TESTS PASSED</b>, tests/results.txt үүснэ (45 PASS).'),
 p('<b>Нэмэлт boundary/error тест (гараар):</b>'),
 code('''./philosophers -n 1            # Error: -n must be in 2..64   (rc=2)
./philosophers -n 65           # Error                          (rc=2)
./philosophers -t 300,100      # Error: -t needs min,max ...    (rc=2)
./philosophers -m 0            # Error                          (rc=2)
./philosophers -n 64 -m 2 -t 5,20 -e 5,20 -q   # дээд хязгаар'''),
 p('<b>Хүлээгдсэн гаралт (-q үед сүүлийн мөр):</b>'),
 code('SUMMARY n=5 meals_each=5 total=25/25 violations=0 max_concurrent_eating=2 ...'),
 p('<i>total = n×meals</i>, <i>violations=0</i>, <i>max_concurrent_eating ≤ n/2</i> байвал зөв. Хугацаа (ms) нь машинаас хамаарна.'),
 p('<b>Known issues:</b>'),
 *bul(['UI нь дискрет симуляц (бодит thread биш); цаг нь simulated.','C програмд Naive (deadlock) горим байхгүй, UI-д л бий.','Monitor шийдэл starvation-ийг албан ёсоор үгүйсгэдэггүй.','UI-г автомат тестлээгүй.'])]
build('docs/Manual.pdf',m)
