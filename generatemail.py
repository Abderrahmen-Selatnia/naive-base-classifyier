import random

# Define some common spam and non-spam phrases
spam_phrases = [
    "Get your free gift now!",
    "Limited time offer, don't miss out!",
    "Act quickly and receive amazing bonuses.",
    "Exclusive deal, only for you!",
    "Don't wait, claim it today before it's too late!",
    "Congratulations, you've won a prize!",
    "Click here to claim your reward.",
    "Urgent: Your account needs verification.",
    "Win big with our lottery!",
    "Special promotion just for you."
]

non_spam_phrases = [
    "Meeting scheduled for tomorrow.",
    "Please find the attached report.",
    "Looking forward to our call next week.",
    "Thank you for your email.",
    "Let's catch up soon.",
    "Project update: All tasks are on track.",
    "Happy birthday! Wishing you a great year ahead.",
    "Can we reschedule our meeting?",
    "Your order has been shipped.",
    "Invoice for your recent purchase."
]

# Generate 2000 emails
emails = []
for i in range(2000):
    # Randomly decide if the email is spam or non-spam
    is_spam = random.choice([True, False])
    
    # Generate a random email address
    email_address = f"user{i}@example.com"
    
    # Select a random phrase based on the spam status
    if is_spam:
        content = random.choice(spam_phrases)
    else:
        content = random.choice(non_spam_phrases)
    
    # Combine the email address and content
    email = f"{email_address},{content}"
    emails.append(email)

# Write the emails to a file
with open("emails.txt", "w") as f:
    for email in emails:
        f.write(email + "\n")
