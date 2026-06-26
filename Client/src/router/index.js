import { createRouter, createWebHashHistory } from 'vue-router'

import AuthLayout from '@/layout/AuthLayout.vue'
import MainLayout from '@/layout/MainLayout.vue'

import LoginView from '@/views/auth/Login.vue'
import RegisterView from '@/views/auth/Register.vue'
import DashboardView from '@/views/dashboard/Dashboard.vue'
import VehicleList from '@/views/vehicle/VehicleList.vue'
import VehicleDetail from '@/views/vehicle/VehicleDetail.vue'
import VehicleForm from '@/views/vehicle/VehicleForm.vue'
import RentalList from '@/views/rental/RentalList.vue'
import RentalCreate from '@/views/rental/RentalCreate.vue'
import RentalDetail from '@/views/rental/RentalDetail.vue'
import MaintenanceList from '@/views/maintenance/MaintenanceList.vue'
import MaintenanceForm from '@/views/maintenance/MaintenanceForm.vue'
import PaymentList from '@/views/finance/PaymentList.vue'
import InvoiceList from '@/views/finance/InvoiceList.vue'
import Statistics from '@/views/stats/Statistics.vue'
import UserProfile from '@/views/user/UserProfile.vue'
import UserList from '@/views/user/UserList.vue'

const ROLE_MAP = { 0: 'customer', 1: 'staff', 2: 'admin' }

const routes = [
  {
    path: '/login',
    component: AuthLayout,
    meta: { guest: true },
    children: [
      { path: '', name: 'Login', component: LoginView },
    ],
  },
  {
    path: '/register',
    component: AuthLayout,
    meta: { guest: true },
    children: [
      { path: '', name: 'Register', component: RegisterView },
    ],
  },
  {
    path: '/',
    component: MainLayout,
    meta: { requiresAuth: true },
    redirect: '/dashboard',
    children: [
      { path: 'dashboard', name: 'Dashboard', component: DashboardView, meta: { title: 'Dashboard' } },
      { path: 'vehicles', name: 'VehicleList', component: VehicleList, meta: { title: 'Vehicles' } },
      { path: 'vehicles/:id', name: 'VehicleDetail', component: VehicleDetail, meta: { title: 'Vehicle Detail' } },
      { path: 'vehicles/add', name: 'VehicleAdd', component: VehicleForm, meta: { title: 'Add Vehicle', roles: ['admin'] } },
      { path: 'vehicles/:id/edit', name: 'VehicleEdit', component: VehicleForm, meta: { title: 'Edit Vehicle', roles: ['admin'] } },
      { path: 'rentals', name: 'RentalList', component: RentalList, meta: { title: 'Rentals' } },
      { path: 'rentals/create', name: 'RentalCreate', component: RentalCreate, meta: { title: 'New Rental' } },
      { path: 'rentals/:id', name: 'RentalDetail', component: RentalDetail, meta: { title: 'Rental Detail' } },
      { path: 'maintenance', name: 'MaintenanceList', component: MaintenanceList, meta: { title: 'Maintenance', roles: ['admin'] } },
      { path: 'maintenance/add', name: 'MaintenanceAdd', component: MaintenanceForm, meta: { title: 'Add Maintenance', roles: ['admin'] } },
      { path: 'maintenance/:id/edit', name: 'MaintenanceEdit', component: MaintenanceForm, meta: { title: 'Edit Maintenance', roles: ['admin'] } },
      { path: 'payments', name: 'PaymentList', component: PaymentList, meta: { title: 'Payments', roles: ['admin'] } },
      { path: 'invoices', name: 'InvoiceList', component: InvoiceList, meta: { title: 'Invoices', roles: ['admin'] } },
      { path: 'statistics', name: 'Statistics', component: Statistics, meta: { title: 'Statistics', roles: ['admin'] } },
      { path: 'profile', name: 'UserProfile', component: UserProfile, meta: { title: 'Profile' } },
      { path: 'users', name: 'UserList', component: UserList, meta: { title: 'Users', roles: ['admin'] } },
    ],
  },
]

const router = createRouter({
  history: createWebHashHistory(),
  routes,
})

router.beforeEach((to, from, next) => {
  const token = localStorage.getItem('token')
  const rawRole = localStorage.getItem('role') || '0'
  const roleKey = ROLE_MAP[rawRole] || 'user'

  if (to.matched.some((r) => r.meta.guest)) {
    if (token) return next('/dashboard')
    return next()
  }

  if (to.matched.some((r) => r.meta.requiresAuth)) {
    if (!token) return next('/login')
    const routeRoles = to.meta.roles
    if (routeRoles && !routeRoles.includes(roleKey)) return next('/dashboard')
  }

  next()
})

export default router
