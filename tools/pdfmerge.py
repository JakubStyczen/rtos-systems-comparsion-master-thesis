from pypdf import PdfReader, PdfWriter


writer = PdfWriter()
reader1 = PdfReader("strona_tyt.pdf")
for page in reader1.pages:
    writer.add_page(page)

reader2 = PdfReader("Magisterka.pdf")

for i in range(1, len(reader2.pages)):
    writer.add_page(reader2.pages[i])

with open("zmergowany_wynik.pdf", "wb") as output_file:
    writer.write(output_file)