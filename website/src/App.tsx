import { I18nProvider } from "@/components/i18n";
import { Landing } from "@/components/landing";

export function App() {
  return (
    <I18nProvider>
      <Landing />
    </I18nProvider>
  );
}
